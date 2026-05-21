/*
 * carm_benchmark.c
 *
 * Controlled Attention Routing and Masking (CARM)
 * Comprehensive Benchmark Suite for Publication
 *
 * Produces CSV data for the following charts:
 *   B1: Routing overhead (%) vs concept count (10–100)
 *   B2: Absolute routing latency vs concept count — O(1) vs O(n) comparison
 *   B3: Attention weight distribution — blocked vs accessible concepts (zero-leakage proof)
 *   B4: Output embedding divergence (cosine distance) vs blocking ratio (0%–100% blocked)
 *   B5: Routing latency distribution (percentiles) — stability under repeated measurement
 *   B6: Overhead % vs allowed/blocked ratio — does overhead depend on policy?
 *   B7: Latency vs rule count — O(1) independence proof across 1–64 rules
 *
 * Author: Horacio López Barrios
 * System: CARM Phases 1.1 and 1.2
 */

#define _POSIX_C_SOURCE 199309L
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>

/* ============================================================
 * Inline routing and attention infrastructure
 * (self-contained: does not depend on external headers
 *  so benchmark compiles standalone)
 * ============================================================ */

#define EMBEDDING_DIM      64
#define NUM_HEADS           8
#define HEAD_DIM           (EMBEDDING_DIM / NUM_HEADS)
#define MAX_CONCEPTS       100
#define ROUTING_TABLE_SIZE 65536
#define WARMUP_ITERS       1000
#define MEASURE_ITERS     10000

typedef uint32_t TOSID;

typedef struct {
    uint8_t actions[ROUTING_TABLE_SIZE];
} RoutingArray;

typedef struct {
    TOSID   tosid;
    float   embedding[EMBEDDING_DIM];
    int     domain;   /* 1=medical, 2=financial, 3=PHI, 0=neutral */
} Concept;

typedef struct {
    float W_query[NUM_HEADS][HEAD_DIM][EMBEDDING_DIM];
    float W_key  [NUM_HEADS][HEAD_DIM][EMBEDDING_DIM];
    float W_value[NUM_HEADS][HEAD_DIM][EMBEDDING_DIM];
    float W_output[EMBEDDING_DIM][EMBEDDING_DIM];
} MHA;

/* ---- timing ---- */
static inline uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* ---- routing ---- */
static void routing_init(RoutingArray *rt) {
    memset(rt->actions, 0, sizeof(rt->actions)); /* DENY all */
}

static void __attribute__((unused)) routing_allow_prefix(RoutingArray *rt, uint32_t prefix16) {
    /* prefix16 = top 16 bits of TOSID = domain<<8 | category */
    rt->actions[prefix16 & 0xFFFF] = 1;
}

static void routing_allow_domain(RoutingArray *rt, uint8_t domain, uint8_t category) {
    uint16_t idx = ((uint16_t)domain << 8) | category;
    rt->actions[idx] = 1;
}

static void routing_allow_all(RoutingArray *rt) {
    memset(rt->actions, 1, sizeof(rt->actions));
}

static inline int routing_check(uint32_t tosid, const RoutingArray *rt) {
    return rt->actions[(tosid >> 16) & 0xFFFF];
}

/* Linear scan baseline */
typedef struct { uint32_t prefix; uint32_t mask; int allow; } Rule;
static int routing_check_linear(uint32_t tosid, const Rule *rules, int nrules) {
    for (int i = 0; i < nrules; i++) {
        if ((tosid & rules[i].mask) == (rules[i].prefix & rules[i].mask))
            return rules[i].allow;
    }
    return 0;
}

/* ---- math helpers ---- */
static float rng_normal(float mean, float std) {
    float u1 = (float)(rand() + 1) / ((float)RAND_MAX + 1);
    float u2 = (float)(rand() + 1) / ((float)RAND_MAX + 1);
    return mean + std * sqrtf(-2.f * logf(u1)) * cosf(2.f * (float)M_PI * u2);
}

static void softmax_inplace(float *v, int n) {
    float mx = v[0];
    for (int i = 1; i < n; i++) if (v[i] > mx) mx = v[i];
    float s = 0.f;
    for (int i = 0; i < n; i++) { v[i] = expf(v[i] - mx); s += v[i]; }
    for (int i = 0; i < n; i++) v[i] /= s;
}

static float dot(const float *a, const float *b, int n) {
    float s = 0.f;
    for (int i = 0; i < n; i++) s += a[i] * b[i];
    return s;
}

static float cosine_dist(const float *a, const float *b, int n) {
    float na = 0.f, nb = 0.f, ab = 0.f;
    for (int i = 0; i < n; i++) {
        na += a[i]*a[i]; nb += b[i]*b[i]; ab += a[i]*b[i];
    }
    na = sqrtf(na); nb = sqrtf(nb);
    if (na < 1e-10f || nb < 1e-10f) return 1.f;
    return 1.f - ab / (na * nb);
}

/* ---- concept generation ---- */
static void make_concept(Concept *c, int domain, int variant) {
    c->domain = domain;
    /* TOSID: domain in byte 3, category=0x05 for med / 0xB1 for fin / 0xD2 for phi */
    uint8_t cat = (domain == 1) ? 0xC5 : (domain == 2) ? 0xB1 : 0xD2;
    c->tosid = ((uint32_t)domain << 24) | ((uint32_t)cat << 16) | (uint32_t)variant;

    srand(domain * 1000 + variant);
    /* domain subspace */
    int ds = (domain - 1) * 21, de = ds + 21;
    if (de > EMBEDDING_DIM) de = EMBEDDING_DIM;
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        c->embedding[i] = 0.05f * ((float)rand()/RAND_MAX - 0.5f);
    }
    for (int i = ds; i < de; i++) {
        c->embedding[i] = 1.f + 0.2f * variant
                          + 0.3f * sinf((float)(i - ds)/3.f + variant);
    }
    float norm = 0.f;
    for (int i = 0; i < EMBEDDING_DIM; i++) norm += c->embedding[i]*c->embedding[i];
    norm = sqrtf(norm);
    for (int i = 0; i < EMBEDDING_DIM; i++) c->embedding[i] /= norm;
}

/* ---- MHA init ---- */
static void mha_init(MHA *m) {
    srand(42);
    float qs = sqrtf(2.f/(EMBEDDING_DIM+HEAD_DIM));
    for (int h = 0; h < NUM_HEADS; h++) {
        srand(42 + h*1000);
        for (int i = 0; i < HEAD_DIM; i++)
            for (int j = 0; j < EMBEDDING_DIM; j++) {
                m->W_query[h][i][j] = rng_normal(0.f, qs);
                m->W_key  [h][i][j] = rng_normal(0.f, qs);
                m->W_value[h][i][j] = rng_normal(0.f, qs);
            }
    }
    srand(42);
    float os = sqrtf(2.f/(EMBEDDING_DIM*2));
    for (int i = 0; i < EMBEDDING_DIM; i++)
        for (int j = 0; j < EMBEDDING_DIM; j++)
            m->W_output[i][j] = rng_normal(0.f, os);
}

/* ---- full attention pass: returns routing_ns and attention_ns ---- */
static void run_attention(
    const float *query,
    Concept *concepts, int nconcepts,
    const RoutingArray *rt,
    const MHA *m,
    float *output,          /* EMBEDDING_DIM */
    uint64_t *routing_ns,
    uint64_t *attention_ns,
    /* optional: per-concept final attention weight aggregated across heads */
    float *attn_weights     /* [nconcepts] or NULL */
) {
    /* Phase 1: routing */
    uint64_t t0 = now_ns();
    Concept *acc[MAX_CONCEPTS];
    int acc_idx[MAX_CONCEPTS]; /* original indices */
    int nacc = 0;
    for (int i = 0; i < nconcepts; i++) {
        if (routing_check(concepts[i].tosid, rt)) {
            acc_idx[nacc] = i;
            acc[nacc++] = &concepts[i];
        }
    }
    *routing_ns = now_ns() - t0;

    if (attn_weights)
        memset(attn_weights, 0, nconcepts * sizeof(float));

    if (nacc == 0) {
        memset(output, 0, EMBEDDING_DIM * sizeof(float));
        *attention_ns = 0;
        return;
    }

    /* Phase 2: attention */
    t0 = now_ns();
    float head_outs[NUM_HEADS][HEAD_DIM];
    float scale = 1.f / sqrtf((float)HEAD_DIM);

    for (int h = 0; h < NUM_HEADS; h++) {
        /* project query */
        float q[HEAD_DIM];
        for (int i = 0; i < HEAD_DIM; i++) {
            q[i] = dot(query, m->W_query[h][i], EMBEDDING_DIM);
        }
        /* project keys and compute scores */
        float scores[MAX_CONCEPTS];
        float keys[MAX_CONCEPTS][HEAD_DIM];
        float vals[MAX_CONCEPTS][HEAD_DIM];
        for (int i = 0; i < nacc; i++) {
            for (int d = 0; d < HEAD_DIM; d++) {
                keys[i][d] = dot(acc[i]->embedding, m->W_key[h][d], EMBEDDING_DIM);
                vals[i][d] = dot(acc[i]->embedding, m->W_value[h][d], EMBEDDING_DIM);
            }
            scores[i] = scale * dot(q, keys[i], HEAD_DIM);
        }
        softmax_inplace(scores, nacc);

        if (attn_weights) {
            for (int i = 0; i < nacc; i++)
                attn_weights[acc_idx[i]] += scores[i] / NUM_HEADS;
        }

        memset(head_outs[h], 0, HEAD_DIM * sizeof(float));
        for (int i = 0; i < nacc; i++)
            for (int d = 0; d < HEAD_DIM; d++)
                head_outs[h][d] += scores[i] * vals[i][d];
    }

    /* Phase 3: concat + project */
    float concat[EMBEDDING_DIM];
    for (int h = 0; h < NUM_HEADS; h++)
        for (int d = 0; d < HEAD_DIM; d++)
            concat[h*HEAD_DIM+d] = head_outs[h][d];

    for (int i = 0; i < EMBEDDING_DIM; i++) {
        output[i] = dot(concat, m->W_output[i], EMBEDDING_DIM);
    }
    *attention_ns = now_ns() - t0;
}

/* ================================================================
 * BENCHMARK B1/B2: Routing overhead and latency vs concept count
 * ================================================================ */
static void bench_b1_b2(const MHA *m) {
    printf("\n# BENCHMARK B1/B2: Routing overhead and latency vs concept count\n");
    printf("# concepts,routing_ns_mean,attention_ns_mean,overhead_pct,"
           "linear_ns_mean,speedup_vs_linear\n");

    /* clinical routing: allow medical (domain=1), block fin(2) and phi(3) */
    RoutingArray rt;
    routing_init(&rt);
    /* allow all domain=1 categories */
    for (int cat = 0; cat < 256; cat++)
        routing_allow_domain(&rt, 1, cat);

    /* linear scan baseline rules */
    Rule rules[3] = {
        { (1u<<24), 0xFF000000, 1 }, /* allow domain 1 */
        { (2u<<24), 0xFF000000, 0 }, /* deny domain 2 */
        { (3u<<24), 0xFF000000, 0 }, /* deny domain 3 */
    };

    int counts[] = {5, 10, 15, 20, 30, 40, 50, 60, 75, 100};
    int ncounts = (int)(sizeof(counts)/sizeof(counts[0]));

    Concept concepts[MAX_CONCEPTS];
    float query[EMBEDDING_DIM];
    srand(99);
    for (int d = 0; d < EMBEDDING_DIM; d++) query[d] = rng_normal(0.f, 1.f);

    /* build concepts: half medical, quarter financial, quarter phi */
    for (int i = 0; i < MAX_CONCEPTS; i++) {
        int dom = (i % 2 == 0) ? 1 : (i % 4 == 1) ? 2 : 3;
        make_concept(&concepts[i], dom, i);
    }

    for (int ci = 0; ci < ncounts; ci++) {
        int nc = counts[ci];
        float out[EMBEDDING_DIM];
        uint64_t rns, ans;

        /* warmup */
        for (int w = 0; w < WARMUP_ITERS; w++)
            run_attention(query, concepts, nc, &rt, m, out, &rns, &ans, NULL);

        /* measure O(1) */
        uint64_t sum_r = 0, sum_a = 0;
        for (int w = 0; w < MEASURE_ITERS; w++) {
            run_attention(query, concepts, nc, &rt, m, out, &rns, &ans, NULL);
            sum_r += rns; sum_a += ans;
        }
        double mean_r = (double)sum_r / MEASURE_ITERS;
        double mean_a = (double)sum_a / MEASURE_ITERS;
        double pct = 100.0 * mean_r / (mean_r + mean_a);

        /* measure linear */
        uint64_t sum_lin = 0;
        for (int w = 0; w < MEASURE_ITERS; w++) {
            uint64_t t0 = now_ns();
            for (int i = 0; i < nc; i++)
                routing_check_linear(concepts[i].tosid, rules, 3);
            sum_lin += now_ns() - t0;
        }
        double mean_lin = (double)sum_lin / MEASURE_ITERS;
        double speedup = mean_lin / mean_r;

        printf("%d,%.2f,%.2f,%.3f,%.2f,%.2f\n",
               nc, mean_r, mean_a, pct, mean_lin, speedup);
    }
}

/* ================================================================
 * BENCHMARK B3: Zero-leakage proof
 * Measures actual attention weight on blocked concepts
 * across many random queries and seeds — must always be 0.000
 * ================================================================ */
static void bench_b3(const MHA *m) {
    printf("\n# BENCHMARK B3: Attention weight on blocked concepts (zero-leakage)\n");
    printf("# trial,max_weight_on_blocked,sum_weight_on_blocked,"
           "min_weight_on_accessible,accessible_count,blocked_count\n");

    RoutingArray rt;
    routing_init(&rt);
    for (int cat = 0; cat < 256; cat++)
        routing_allow_domain(&rt, 1, cat); /* allow only medical */

    Concept concepts[10];
    /* 5 medical, 3 financial, 2 phi */
    for (int i = 0; i < 5; i++)  make_concept(&concepts[i],   1, i);
    for (int i = 0; i < 3; i++)  make_concept(&concepts[5+i], 2, i);
    for (int i = 0; i < 2; i++)  make_concept(&concepts[8+i], 3, i);

    int TRIALS = 1000;
    int violations = 0;
    for (int t = 0; t < TRIALS; t++) {
        srand(t * 31337 + 42);
        float query[EMBEDDING_DIM];
        for (int d = 0; d < EMBEDDING_DIM; d++) query[d] = rng_normal(0.f, 1.f);

        float out[EMBEDDING_DIM];
        uint64_t rns, ans;
        float weights[10];
        run_attention(query, concepts, 10, &rt, m, out, &rns, &ans, weights);

        float max_blocked = 0.f, sum_blocked = 0.f, min_acc = 1.f;
        int nacc = 0;
        for (int i = 0; i < 10; i++) {
            if (concepts[i].domain == 1) {
                if (weights[i] < min_acc) min_acc = weights[i];
                nacc++;
            } else {
                if (weights[i] > max_blocked) max_blocked = weights[i];
                sum_blocked += weights[i];
                if (weights[i] > 1e-10f) violations++;
            }
        }
        printf("%d,%.10f,%.10f,%.6f,%d,%d\n",
               t, max_blocked, sum_blocked, min_acc, nacc, 10-nacc);
    }
    printf("# Total violations (weight > 1e-10 on blocked): %d / %d\n",
           violations, TRIALS * 5);
}

/* ================================================================
 * BENCHMARK B4: Output embedding divergence vs blocking ratio
 * Proves routing changes output meaningfully (not noise)
 * ================================================================ */
static void bench_b4(const MHA *m) {
    printf("\n# BENCHMARK B4: Output cosine distance vs blocking ratio\n");
    printf("# blocked_pct,cosine_dist_from_unrestricted,accessible_count\n");

    /* 10 concepts: 10 medical */
    Concept concepts[10];
    for (int i = 0; i < 10; i++) make_concept(&concepts[i], 1, i);

    float query[EMBEDDING_DIM];
    srand(42);
    for (int d = 0; d < EMBEDDING_DIM; d++) query[d] = rng_normal(0.f, 1.f);

    /* baseline: all accessible */
    RoutingArray rt_all;
    routing_init(&rt_all);
    routing_allow_all(&rt_all);
    float out_all[EMBEDDING_DIM];
    uint64_t rns, ans;
    run_attention(query, concepts, 10, &rt_all, m, out_all, &rns, &ans, NULL);

    /* vary: block 0, 1, 2, ... 9 concepts */
    for (int nblocked = 0; nblocked <= 10; nblocked++) {
        /* allow first (10-nblocked) concepts, block the rest */
        /* we assign unique prefixes per concept for fine control */
        RoutingArray rt;
        routing_init(&rt);
        for (int i = 0; i < 10 - nblocked; i++) {
            
            /* ensure each concept has unique prefix by using variant */
            /* rebuild tosid so top 16 bits are unique */
            concepts[i].tosid = ((uint32_t)1 << 24) | ((uint32_t)(0xC0 + i) << 16) | (uint32_t)i;
            rt.actions[(concepts[i].tosid >> 16) & 0xFFFF] = 1;
        }
        for (int i = 10 - nblocked; i < 10; i++) {
            concepts[i].tosid = ((uint32_t)1 << 24) | ((uint32_t)(0xC0 + i) << 16) | (uint32_t)i;
            rt.actions[(concepts[i].tosid >> 16) & 0xFFFF] = 0;
        }

        float out[EMBEDDING_DIM];
        run_attention(query, concepts, 10, &rt, m, out, &rns, &ans, NULL);

        float dist = cosine_dist(out_all, out, EMBEDDING_DIM);
        int accessible = 10 - nblocked;
        printf("%d,%.6f,%d\n", nblocked * 10, dist, accessible);
    }
}

/* ================================================================
 * BENCHMARK B5: Latency distribution (percentiles) — stability
 * ================================================================ */
#define N_SAMPLES 20000
static int cmp_u64(const void *a, const void *b) {
    uint64_t x = *(uint64_t*)a, y = *(uint64_t*)b;
    return (x > y) - (x < y);
}

static void bench_b5(const MHA *m) {
    printf("\n# BENCHMARK B5: Routing latency distribution (ns) over %d samples\n", N_SAMPLES);
    printf("# p1,p5,p10,p25,p50,p75,p90,p95,p99,p999,mean,min,max\n");

    RoutingArray rt;
    routing_init(&rt);
    for (int cat = 0; cat < 256; cat++)
        routing_allow_domain(&rt, 1, cat);

    Concept concepts[10];
    for (int i = 0; i < 5;  i++) make_concept(&concepts[i],   1, i);
    for (int i = 0; i < 3;  i++) make_concept(&concepts[5+i], 2, i);
    for (int i = 0; i < 2;  i++) make_concept(&concepts[8+i], 3, i);

    float query[EMBEDDING_DIM], out[EMBEDDING_DIM];
    srand(42);
    for (int d = 0; d < EMBEDDING_DIM; d++) query[d] = rng_normal(0.f, 1.f);

    uint64_t *samples = malloc(N_SAMPLES * sizeof(uint64_t));
    uint64_t rns, ans;

    /* warmup */
    for (int w = 0; w < WARMUP_ITERS; w++)
        run_attention(query, concepts, 10, &rt, m, out, &rns, &ans, NULL);

    for (int i = 0; i < N_SAMPLES; i++) {
        run_attention(query, concepts, 10, &rt, m, out, &rns, &ans, NULL);
        samples[i] = rns;
    }

    qsort(samples, N_SAMPLES, sizeof(uint64_t), cmp_u64);

    uint64_t sum = 0;
    for (int i = 0; i < N_SAMPLES; i++) sum += samples[i];
    double mean = (double)sum / N_SAMPLES;

    #define PCT(p) (samples[(int)((p)/100.0*N_SAMPLES)])
    printf("%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64
           ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64
           ",%" PRIu64 ",%" PRIu64 ",%.2f,%" PRIu64 ",%" PRIu64 "\n",
           PCT(1), PCT(5), PCT(10), PCT(25), PCT(50),
           PCT(75), PCT(90), PCT(95), PCT(99), PCT(99.9),
           mean, samples[0], samples[N_SAMPLES-1]);
    #undef PCT

    free(samples);
}

/* ================================================================
 * BENCHMARK B6: Overhead % vs allowed/blocked ratio
 * ================================================================ */
static void bench_b6(const MHA *m) {
    printf("\n# BENCHMARK B6: Routing overhead %% vs allowed fraction\n");
    printf("# allowed_pct,routing_ns,attention_ns,overhead_pct\n");

    /* 10 concepts, all distinct prefixes */
    Concept concepts[10];
    for (int i = 0; i < 10; i++) {
        make_concept(&concepts[i], 1, i);
        /* unique top-16 prefix per concept */
        concepts[i].tosid = ((uint32_t)0x10 << 24) |
                            ((uint32_t)(0xA0 + i) << 16) |
                            (uint32_t)i;
    }

    float query[EMBEDDING_DIM], out[EMBEDDING_DIM];
    srand(42);
    for (int d = 0; d < EMBEDDING_DIM; d++) query[d] = rng_normal(0.f, 1.f);

    for (int nallowed = 0; nallowed <= 10; nallowed++) {
        RoutingArray rt;
        routing_init(&rt);
        for (int i = 0; i < nallowed; i++)
            rt.actions[(concepts[i].tosid >> 16) & 0xFFFF] = 1;

        uint64_t rns, ans;
        /* warmup */
        for (int w = 0; w < WARMUP_ITERS; w++)
            run_attention(query, concepts, 10, &rt, m, out, &rns, &ans, NULL);

        uint64_t sum_r = 0, sum_a = 0;
        for (int w = 0; w < MEASURE_ITERS; w++) {
            run_attention(query, concepts, 10, &rt, m, out, &rns, &ans, NULL);
            sum_r += rns; sum_a += ans;
        }
        double mr = (double)sum_r/MEASURE_ITERS;
        double ma = (double)sum_a/MEASURE_ITERS;
        double pct = (ma < 1.0) ? 100.0 : 100.0 * mr / (mr + ma);
        printf("%d,%.2f,%.2f,%.3f\n", nallowed*10, mr, ma, pct);
    }
}

/* ================================================================
 * BENCHMARK B7: O(1) independence — latency vs rule count
 * ================================================================ */
static void bench_b7(void) {
    printf("\n# BENCHMARK B7: Routing latency vs rule count (O(1) proof)\n");
    printf("# nrules,o1_ns,linear_ns,speedup\n");

    Concept concepts[10];
    for (int i = 0; i < 5;  i++) make_concept(&concepts[i],   1, i);
    for (int i = 0; i < 3;  i++) make_concept(&concepts[5+i], 2, i);
    for (int i = 0; i < 2;  i++) make_concept(&concepts[8+i], 3, i);

    int rule_counts[] = {1,2,4,8,16,32,48,64};
    int nr = (int)(sizeof(rule_counts)/sizeof(rule_counts[0]));

    for (int ri = 0; ri < nr; ri++) {
        int nrules = rule_counts[ri];

        /* Build O(1) table */
        RoutingArray rt;
        routing_init(&rt);
        for (int cat = 0; cat < 256; cat++)
            routing_allow_domain(&rt, 1, cat);

        /* Build linear rules (nrules rules, first allows medical, rest are redundant denies) */
        Rule *rules = calloc(nrules, sizeof(Rule));
        rules[0].prefix = (1u<<24); rules[0].mask = 0xFF000000; rules[0].allow = 1;
        for (int i = 1; i < nrules; i++) {
            rules[i].prefix = ((uint32_t)(0xF0+i)<<24);
            rules[i].mask   = 0xFF000000;
            rules[i].allow  = 0;
        }

        /* warmup O(1) */
        for (int w = 0; w < WARMUP_ITERS; w++)
            for (int i = 0; i < 10; i++) routing_check(concepts[i].tosid, &rt);

        /* measure O(1) */
        uint64_t sum_o1 = 0;
        for (int w = 0; w < MEASURE_ITERS; w++) {
            uint64_t t0 = now_ns();
            for (int i = 0; i < 10; i++) routing_check(concepts[i].tosid, &rt);
            sum_o1 += now_ns() - t0;
        }

        /* warmup linear */
        for (int w = 0; w < WARMUP_ITERS; w++)
            for (int i = 0; i < 10; i++)
                routing_check_linear(concepts[i].tosid, rules, nrules);

        /* measure linear */
        uint64_t sum_lin = 0;
        for (int w = 0; w < MEASURE_ITERS; w++) {
            uint64_t t0 = now_ns();
            for (int i = 0; i < 10; i++)
                routing_check_linear(concepts[i].tosid, rules, nrules);
            sum_lin += now_ns() - t0;
        }

        double o1  = (double)sum_o1  / MEASURE_ITERS;
        double lin = (double)sum_lin / MEASURE_ITERS;
        printf("%d,%.2f,%.2f,%.2f\n", nrules, o1, lin, lin/o1);

        free(rules);
    }
}

/* ================================================================
 * main
 * ================================================================ */
int main(void) {
    MHA m;
    mha_init(&m);

    printf("# CARM Benchmark Suite\n");
    printf("# Controlled Attention Routing and Masking\n");
    printf("# All times in nanoseconds unless stated\n");

    bench_b1_b2(&m);
    bench_b3(&m);
    bench_b4(&m);
    bench_b5(&m);
    bench_b6(&m);
    bench_b7();

    return 0;
}
