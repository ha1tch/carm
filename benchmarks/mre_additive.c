/*
 * mre_additive.c
 *
 * CARM — Controlled Attention Routing and Masking
 * Micro-Universe Minimal Reproducible Example: Additive Masking Variant
 *
 * This file implements the canonical 4-concept, 4-dimension worked example
 * using the additive -infinity masking approach (Stage II, Track A).
 * It is the ground-truth test case for any additive masking implementation
 * of CARM, and serves as the opening illustration in Paper 2.
 *
 * The parameters are fixed and analytically determined:
 *
 *   Vocabulary (4 concepts):
 *     C1: Aspirin  (medical,    TOSID 0x10000001)
 *     C2: Billing  (financial,  TOSID 0x20000001)  <- blocked in clinical policy
 *     C3: SSN      (PHI,        TOSID 0x30000001)  <- blocked in clinical policy
 *     C4: System   (neutral,    TOSID 0x00000001)
 *
 *   Embeddings: one-hot orthogonal (identity basis vectors)
 *     E1 = [1, 0, 0, 0]
 *     E2 = [0, 1, 0, 0]
 *     E3 = [0, 0, 1, 0]
 *     E4 = [0, 0, 0, 1]
 *
 *   Weight matrices: identity (W_q = W_k = W_v = I)
 *     K = V = E  (projections are identity: no distortion)
 *
 *   Adversarial query: maximum affinity toward C2 (Billing)
 *     Q = [0, 10, 0, 0]  (scaled x10 for sharp softmax)
 *
 *   Expected outputs:
 *     Scenario A (unrestricted):        [0.007, 0.980, 0.007, 0.007]
 *       (scale factor 1/sqrt(4)=0.5 applied; softmax of [0,5,0,0])
 *     Scenario B (clinical, C2+C3 blocked): [0.500, 0.000, 0.000, 0.500]
 *
 * NOTE: This file is a Stage II placeholder. The Stage I implementation
 * uses array compaction (accessible concepts only). This file implements
 * the additive masking variant operating over the full static array,
 * which is the hardware-compatible form for GPU/TPU dense tensor operations.
 *
 * Compile:
 *   gcc -O2 -Wall -std=c11 -o mre_additive mre_additive.c -lm
 *
 * Author: Horacio López Barrios
 * Stage: II (Track A) — additive masking implementation
 */

#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include <assert.h>

/* ── Constants ──────────────────────────────────────────────────────────── */
#define N_CONCEPTS  4
#define N_DIM       4
#define SCALE       (1.0f / 2.0f)   /* 1/sqrt(d) = 1/sqrt(4) = 0.5 */
#define NEG_INF     (-1.0f / 0.0f)  /* -infinity for additive mask */

/* TOSID domain byte (upper 8 bits of 32-bit TOSID) */
#define DOMAIN_NEUTRAL   0x00
#define DOMAIN_MEDICAL   0x10
#define DOMAIN_FINANCIAL 0x20
#define DOMAIN_PHI       0x30

/* ── Types ──────────────────────────────────────────────────────────────── */
typedef unsigned int uint32_t_alias;

typedef struct {
    const char  *name;
    uint32_t_alias tosid;
    float        embedding[N_DIM];
} Concept;

/* ── Vocabulary ─────────────────────────────────────────────────────────── */
static const Concept vocab[N_CONCEPTS] = {
    { "Aspirin", 0x10000001, { 1.0f, 0.0f, 0.0f, 0.0f } },
    { "Billing", 0x20000001, { 0.0f, 1.0f, 0.0f, 0.0f } },
    { "SSN",     0x30000001, { 0.0f, 0.0f, 1.0f, 0.0f } },
    { "System",  0x00000001, { 0.0f, 0.0f, 0.0f, 1.0f } },
};

/* ── Routing policy ─────────────────────────────────────────────────────── */

/*
 * Returns 1 (ALLOW) or 0 (DENY) for a given TOSID under the named policy.
 * Policy is determined solely by the domain byte (upper 8 bits).
 * This is the O(1) lookup: in the full implementation this is a table
 * array[tosid >> 24]; here it is written explicitly for clarity.
 */
static int route(uint32_t_alias tosid, int clinical_policy) {
    unsigned int domain = (tosid >> 24) & 0xFF;
    if (!clinical_policy) return 1;  /* unrestricted: allow all */
    /* Clinical policy: allow medical and neutral; deny financial and PHI */
    return (domain == DOMAIN_MEDICAL || domain == DOMAIN_NEUTRAL) ? 1 : 0;
}

/* ── Softmax over a static array (additive masking variant) ─────────────── */

/*
 * Applies softmax to logits[] of length n.
 * Entries with value NEG_INF produce weight 0.0 exactly.
 * The array is NOT compacted: static geometry is preserved throughout.
 */
static void softmax_static(float *logits, float *weights, int n) {
    /* Numerical stability: subtract max of finite values */
    float mx = -FLT_MAX;
    for (int i = 0; i < n; i++)
        if (logits[i] > -FLT_MAX && logits[i] > mx) mx = logits[i];

    float sum = 0.0f;
    for (int i = 0; i < n; i++) {
        if (logits[i] <= NEG_INF) {
            weights[i] = 0.0f;
        } else {
            weights[i] = expf(logits[i] - mx);
            sum += weights[i];
        }
    }
    for (int i = 0; i < n; i++)
        if (weights[i] > 0.0f) weights[i] /= sum;
}

/* ── Single-head attention with additive CARM mask ──────────────────────── */

/*
 * Computes attention over the full static vocabulary.
 * Phase 1: build additive mask M from routing policy (query-independent).
 * Phase 2: compute logits = Q . K^T, apply M, softmax.
 * Phase 3: weighted sum of value vectors.
 *
 * The tensor shape (N_CONCEPTS x N_DIM) never changes.
 * Blocked concepts receive logit += -inf, producing weight 0.0 exactly.
 */
static void carm_attention(
    const float query[N_DIM],
    int         clinical_policy,
    float       output[N_DIM],
    float       weights_out[N_CONCEPTS],   /* for inspection */
    float       mask_out[N_CONCEPTS]       /* for inspection */
) {
    /* ── Phase 1: Routing (query-independent) ── */
    float mask[N_CONCEPTS];
    int   n_blocked = 0;
    for (int i = 0; i < N_CONCEPTS; i++) {
        mask[i] = route(vocab[i].tosid, clinical_policy) ? 0.0f : NEG_INF;
        if (mask[i] == NEG_INF) n_blocked++;
        if (mask_out) mask_out[i] = mask[i];
    }
    (void)n_blocked;  /* used below in assertions */

    /* ── Phase 2: Attention (over full static array) ── */
    float logits[N_CONCEPTS];
    for (int i = 0; i < N_CONCEPTS; i++) {
        /* dot(Q, K_i) * scale; with W_k = I, K_i = E_i */
        float dot = 0.0f;
        for (int d = 0; d < N_DIM; d++)
            dot += query[d] * vocab[i].embedding[d];
        logits[i] = dot * SCALE + mask[i];   /* additive mask applied here */
    }

    float weights[N_CONCEPTS];
    softmax_static(logits, weights, N_CONCEPTS);
    if (weights_out) memcpy(weights_out, weights, N_CONCEPTS * sizeof(float));

    /* ── Phase 3: Weighted sum (full static array) ── */
    memset(output, 0, N_DIM * sizeof(float));
    for (int i = 0; i < N_CONCEPTS; i++)
        for (int d = 0; d < N_DIM; d++)
            output[d] += weights[i] * vocab[i].embedding[d];
}

/* ── Printing helpers ───────────────────────────────────────────────────── */
static void print_vec(const char *label, const float *v, int n) {
    printf("  %-30s [", label);
    for (int i = 0; i < n; i++)
        printf("%6.3f%s", v[i], i < n-1 ? ", " : "");
    printf("]\n");
}

static void print_weights(const float *w, const float *m) {
    printf("  %-30s\n", "Attention weights:");
    for (int i = 0; i < N_CONCEPTS; i++) {
        const char *status = (m[i] == NEG_INF) ? " [BLOCKED]" : " [allowed]";
        printf("    %-10s %.4f%s\n", vocab[i].name, w[i], status);
    }
}

/* ── Verification against analytic expected values ─────────────────────── */

static int approx_eq(float a, float b, float tol) {
    return fabsf(a - b) < tol;
}

static void verify_scenario_a(const float *out, const float *w) {
    /* Unrestricted: Q = [0,10,0,0] -> attends entirely to C2 (Billing) */
    assert(approx_eq(w[0], 0.0f,  0.01f) && "C1 weight should be ~0");
    assert(approx_eq(w[1], 0.98f, 0.01f) && "C2 weight should be ~0.98 (scaled dot product)");
    assert(approx_eq(w[2], 0.0f,  0.01f) && "C3 weight should be ~0");
    assert(approx_eq(w[3], 0.0f,  0.01f) && "C4 weight should be ~0");
    assert(approx_eq(out[1], 0.98f, 0.01f) && "Output dim 1 should be ~0.98");
    printf("  PASS: Scenario A analytic verification\n");
}

static void verify_scenario_b(const float *out, const float *w) {
    /* Clinical: C2 and C3 blocked. Q has zero affinity for C1 and C4,
     * so weight distributes evenly: 0.5 each. */
    assert(approx_eq(w[1], 0.0f,  1e-6f) && "C2 (Billing) weight must be exactly 0");
    assert(approx_eq(w[2], 0.0f,  1e-6f) && "C3 (SSN) weight must be exactly 0");
    assert(approx_eq(w[0], 0.5f,  0.01f) && "C1 weight should be 0.5");
    assert(approx_eq(w[3], 0.5f,  0.01f) && "C4 weight should be 0.5");
    assert(approx_eq(out[0], 0.5f, 0.01f) && "Output dim 0 should be 0.5");
    assert(approx_eq(out[1], 0.0f, 1e-6f) && "Output dim 1 must be exactly 0");
    assert(approx_eq(out[3], 0.5f, 0.01f) && "Output dim 3 should be 0.5");
    printf("  PASS: Scenario B analytic verification (blocked weight = 0.000000)\n");
}

/* ── Main ───────────────────────────────────────────────────────────────── */
int main(void) {
    printf("\n");
    printf("============================================================\n");
    printf("  CARM Micro-Universe MRE — Additive Masking Variant\n");
    printf("  Stage II, Track A — canonical ground-truth test\n");
    printf("============================================================\n\n");

    printf("Vocabulary:\n");
    for (int i = 0; i < N_CONCEPTS; i++)
        printf("  C%d: %-10s  TOSID 0x%08X  E%d = one-hot(%d)\n",
               i+1, vocab[i].name, vocab[i].tosid, i+1, i);

    float query[N_DIM]   = { 0.0f, 10.0f, 0.0f, 0.0f };
    float output[N_DIM];
    float weights[N_CONCEPTS];
    float mask[N_CONCEPTS];

    printf("\nAdversarial query Q = [0, 10, 0, 0]\n");
    printf("(Maximum affinity toward C2: Billing)\n");
    printf("Scale factor: 1/sqrt(%d) = %.4f\n\n", N_DIM, SCALE);

    /* ── Scenario A: Unrestricted ── */
    printf("------------------------------------------------------------\n");
    printf("Scenario A: Unrestricted policy (all concepts accessible)\n");
    printf("------------------------------------------------------------\n");
    carm_attention(query, 0, output, weights, mask);
    print_vec("Mask M:", mask, N_CONCEPTS);
    print_vec("Logits + M:", (float[]){
        query[0]*SCALE + mask[0],
        query[1]*SCALE + mask[1],
        query[2]*SCALE + mask[2],
        query[3]*SCALE + mask[3]}, N_CONCEPTS);
    print_weights(weights, mask);
    print_vec("Output:", output, N_DIM);
    verify_scenario_a(output, weights);

    printf("\n");

    /* ── Scenario B: Clinical policy ── */
    printf("------------------------------------------------------------\n");
    printf("Scenario B: Clinical policy (Financial + PHI blocked)\n");
    printf("------------------------------------------------------------\n");
    carm_attention(query, 1, output, weights, mask);
    print_vec("Mask M:", mask, N_CONCEPTS);

    /* Print logits manually since -inf needs special formatting */
    printf("  %-30s [", "Logits + M:");
    for (int i = 0; i < N_CONCEPTS; i++) {
        float l = query[i]*SCALE + mask[i];
        if (l <= NEG_INF)
            printf("  -inf%s", i < N_CONCEPTS-1 ? ", " : "");
        else
            printf("%6.3f%s", l, i < N_CONCEPTS-1 ? ", " : "");
    }
    printf("]\n");
    print_weights(weights, mask);
    print_vec("Output:", output, N_DIM);
    verify_scenario_b(output, weights);

    printf("\n");
    printf("============================================================\n");
    printf("  Both scenarios verified against analytic expected values.\n");
    printf("  C1 (Zero weight) holds exactly for all blocked concepts.\n");
    printf("  Tensor shape: static throughout (no array compaction).\n");
    printf("============================================================\n\n");

    return 0;
}
