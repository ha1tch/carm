/*
 * multihead_attention.c
 * 
 * Multi-Head Attention with TOSID-based Routing Integration
 * Core Implementation
 * 
 * CARM Proof-of-Concept Implementation
 */
/*
 * multihead_attention.c
 *
 * CARM -- Controlled Attention Routing and Masking
 * Three-phase CARM computation: routing, attention, output
 *
 * Copyright (c) 2026 haitch
 * Licensed under the Apache License, Version 2.0
 * https://www.apache.org/licenses/LICENSE-2.0
 */


#define _POSIX_C_SOURCE 199309L
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "multihead_attention.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <inttypes.h>

// ============================================================================
// ROUTING TABLE FUNCTIONS
// ============================================================================

// ============================================================================
// O(1) Routing Implementation
// ============================================================================

void build_fast_routing_table(RoutingTable* table) {
    // Initialize fast lookup table (default: DENY all)
    routing_table_init(&table->fast_lookup, table->context_name);
    
    // Convert each rule into the fast lookup table
    for (int i = 0; i < table->rule_count; i++) {
        FastRoutingRule fast_rule;
        fast_rule.prefix = table->rules[i].prefix;
        fast_rule.mask = table->rules[i].mask;
        fast_rule.action = (table->rules[i].action == ALLOW) ? ROUTE_ALLOW : ROUTE_DENY;
        
        routing_table_add_rule(&table->fast_lookup, &fast_rule);
    }
}

int check_routing_access(TOSID tosid, RoutingTable* table) {
    // Use O(1) fast lookup table
    // This replaces the linear scan with constant-time array lookup
    return routing_table_check(tosid, &table->fast_lookup);
}

// Old linear scan implementation (kept for reference, not used)
int check_routing_access_linear_DEPRECATED(TOSID tosid, RoutingTable* table) {
    // Default deny if no rules match
    int result = 0;
    
    for (int i = 0; i < table->rule_count; i++) {
        // Apply mask and compare prefix
        if ((tosid & table->rules[i].mask) == 
            (table->rules[i].prefix & table->rules[i].mask)) {
            result = (table->rules[i].action == ALLOW) ? 1 : 0;
            break;  // First match wins
        }
    }
    
    return result;
}

void init_routing_clinical(RoutingTable* table) {
    strcpy(table->context_name, "Clinical Context");
    table->rule_count = 3;
    
    // Rule 1: Allow medical concepts
    table->rules[0].prefix = TOSID_MEDICAL_PREFIX;
    table->rules[0].mask = TOSID_MEDICAL_MASK;
    table->rules[0].action = ALLOW;
    strcpy(table->rules[0].description, "Allow medical concepts");
    strcpy(table->rules[0].tosid_pattern, "10C-MED-***");
    
    // Rule 2: Block financial concepts
    table->rules[1].prefix = TOSID_FINANCIAL_PREFIX;
    table->rules[1].mask = TOSID_FINANCIAL_MASK;
    table->rules[1].action = DENY;
    strcpy(table->rules[1].description, "Block financial concepts");
    strcpy(table->rules[1].tosid_pattern, "11B-FIN-***");
    
    // Rule 3: Block PHI
    table->rules[2].prefix = TOSID_PHI_PREFIX;
    table->rules[2].mask = TOSID_PHI_MASK;
    table->rules[2].action = DENY;
    strcpy(table->rules[2].description, "Block PHI");
    strcpy(table->rules[2].tosid_pattern, "11B-PER-IDN-***");
    
    // Build O(1) fast lookup table
    build_fast_routing_table(table);
}

void init_routing_unrestricted(RoutingTable* table) {
    strcpy(table->context_name, "Unrestricted Context");
    table->rule_count = 1;
    
    // Allow everything
    table->rules[0].prefix = 0x00000000;
    table->rules[0].mask = 0x00000000;
    table->rules[0].action = ALLOW;
    strcpy(table->rules[0].description, "Allow all concepts");
    strcpy(table->rules[0].tosid_pattern, "***-***-***");
    
    // Build O(1) fast lookup table
    build_fast_routing_table(table);
}

void init_routing_deny_all(RoutingTable* table) {
    strcpy(table->context_name, "Deny All Context");
    table->rule_count = 1;
    
    // Block everything
    table->rules[0].prefix = 0x00000000;
    table->rules[0].mask = 0x00000000;
    table->rules[0].action = DENY;
    strcpy(table->rules[0].description, "Block all concepts");
    strcpy(table->rules[0].tosid_pattern, "***-***-***");
    
    // Build O(1) fast lookup table
    build_fast_routing_table(table);
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

float random_normal(float mean, float stddev) {
    // Box-Muller transform
    float u1 = (float)rand() / RAND_MAX;
    float u2 = (float)rand() / RAND_MAX;
    
    // Avoid log(0)
    if (u1 < 1e-10f) u1 = 1e-10f;
    
    float z = sqrtf(-2.0f * logf(u1)) * cosf(2.0f * M_PI * u2);
    return mean + stddev * z;
}

uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

void project_embedding(const float* input, 
                      float* output,
                      float weights[][EMBEDDING_DIM],
                      int output_dim) {
    for (int i = 0; i < output_dim; i++) {
        output[i] = 0.0f;
        for (int j = 0; j < EMBEDDING_DIM; j++) {
            output[i] += weights[i][j] * input[j];
        }
    }
}

void softmax(float* scores, int count) {
    if (count == 0) return;
    
    // Find max for numerical stability
    float max_score = scores[0];
    for (int i = 1; i < count; i++) {
        if (scores[i] > max_score) {
            max_score = scores[i];
        }
    }
    
    // Compute exp(x - max) and sum
    float sum = 0.0f;
    for (int i = 0; i < count; i++) {
        scores[i] = expf(scores[i] - max_score);
        sum += scores[i];
    }
    
    // Normalise
    if (sum > 0.0f) {
        for (int i = 0; i < count; i++) {
            scores[i] /= sum;
        }
    }
}

void find_topk_concepts(float* scores,
                       Concept** concepts,
                       int count,
                       int* top_indices,
                       float* top_weights,
                       int k) {
    // Create temporary arrays for sorting
    float* temp_scores = malloc(count * sizeof(float));
    Concept** temp_concepts = malloc(count * sizeof(Concept*));
    
    memcpy(temp_scores, scores, count * sizeof(float));
    memcpy(temp_concepts, concepts, count * sizeof(Concept*));
    
    // Simple selection sort for top-k
    for (int i = 0; i < k && i < count; i++) {
        int max_idx = i;
        float max_score = temp_scores[i];
        
        for (int j = i + 1; j < count; j++) {
            if (temp_scores[j] > max_score) {
                max_idx = j;
                max_score = temp_scores[j];
            }
        }
        
        // Swap
        if (max_idx != i) {
            float temp_score = temp_scores[i];
            temp_scores[i] = temp_scores[max_idx];
            temp_scores[max_idx] = temp_score;
            
            Concept* temp_concept = temp_concepts[i];
            temp_concepts[i] = temp_concepts[max_idx];
            temp_concepts[max_idx] = temp_concept;
        }
        
        // Store result - we need to find the original index
        // The pointer difference gives us the index
        // concepts[0] is the base pointer to the concept array
        top_indices[i] = (int)(temp_concepts[i] - concepts[0]);
        top_weights[i] = temp_scores[i];
    }
    
    free(temp_scores);
    free(temp_concepts);
}

float cosine_similarity(const float* a, const float* b, int dim) {
    float dot = 0.0f;
    float norm_a = 0.0f;
    float norm_b = 0.0f;
    
    for (int i = 0; i < dim; i++) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    
    norm_a = sqrtf(norm_a);
    norm_b = sqrtf(norm_b);
    
    if (norm_a < 1e-10f || norm_b < 1e-10f) {
        return 0.0f;
    }
    
    return dot / (norm_a * norm_b);
}

// ============================================================================
// MULTI-HEAD ATTENTION FUNCTIONS
// ============================================================================

void init_multihead_attention(MultiHeadAttention* mha) {
    mha->num_heads = NUM_HEADS;
    mha->head_dim = HEAD_DIM;
    mha->embedding_dim = EMBEDDING_DIM;
    
    // Xavier initialisation: scale = sqrt(2.0 / (fan_in + fan_out))
    float query_scale = sqrtf(2.0f / (EMBEDDING_DIM + HEAD_DIM));
    float key_scale = sqrtf(2.0f / (EMBEDDING_DIM + HEAD_DIM));
    float value_scale = sqrtf(2.0f / (EMBEDDING_DIM + HEAD_DIM));
    float output_scale = sqrtf(2.0f / (EMBEDDING_DIM * 2));
    
    // Initialise each head's projection matrices
    // Use head-specific seeds for more diversity
    for (int h = 0; h < NUM_HEADS; h++) {
        // Add head-specific randomness
        srand(42 + h * 1000);
        
        for (int i = 0; i < HEAD_DIM; i++) {
            for (int j = 0; j < EMBEDDING_DIM; j++) {
                mha->W_query[h][i][j] = random_normal(0.0f, query_scale);
                mha->W_key[h][i][j] = random_normal(0.0f, key_scale);
                mha->W_value[h][i][j] = random_normal(0.0f, value_scale);
            }
        }
    }
    
    // Reset seed for output projection
    srand(42);
    
    // Initialise output projection
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        for (int j = 0; j < EMBEDDING_DIM; j++) {
            mha->W_output[i][j] = random_normal(0.0f, output_scale);
        }
    }
    
    printf("Multi-head attention initialised:\n");
    printf("  Heads: %d\n", NUM_HEADS);
    printf("  Head dimension: %d\n", HEAD_DIM);
    printf("  Embedding dimension: %d\n", EMBEDDING_DIM);
}

void compute_multihead_attention_with_routing(
    float* query_embedding,
    Concept* concepts,
    int concept_count,
    RoutingTable* routing_table,
    MultiHeadAttention* mha,
    MultiHeadAttentionResult* result
) {
    uint64_t start_time = get_time_ns();
    
    // Initialise result
    memset(result, 0, sizeof(MultiHeadAttentionResult));
    
    // Store routing context name for reporting
    memcpy(result->routing_context_name, routing_table->context_name, 63);
    result->routing_context_name[63] = '\0';
    
    // PHASE 1: ROUTING ENFORCEMENT (ONCE FOR ALL HEADS)
    uint64_t routing_start = get_time_ns();
    
    // Filter concepts based on routing table
    Concept* accessible_concepts[MAX_CONCEPTS];
    int accessible_count = 0;
    int blocked_count = 0;
    
    // Store routing trace data (no printing during computation)
    result->trace.routing_trace.count = 0;
    
    for (int i = 0; i < concept_count; i++) {
        int accessible = check_routing_access(concepts[i].tosid, routing_table);
        
        // Store trace data
        int trace_idx = result->trace.routing_trace.count++;
        result->trace.routing_trace.tosids[trace_idx] = concepts[i].tosid;
        memcpy(result->trace.routing_trace.names[trace_idx], concepts[i].name, 63);
        result->trace.routing_trace.names[trace_idx][63] = '\0';
        memcpy(result->trace.routing_trace.tosid_full[trace_idx], concepts[i].tosid_full, 63);
        result->trace.routing_trace.tosid_full[trace_idx][63] = '\0';
        result->trace.routing_trace.accessible[trace_idx] = accessible;
        
        if (accessible) {
            accessible_concepts[accessible_count++] = &concepts[i];
        } else {
            result->blocked_tosids[blocked_count++] = concepts[i].tosid;
        }
    }
    
    result->total_accessible_concepts = accessible_count;
    result->total_blocked_concepts = blocked_count;
    result->num_blocked = blocked_count;
    result->routing_time_ns = get_time_ns() - routing_start;
    
    // Early exit if no accessible concepts
    if (accessible_count == 0) {
        result->total_time_ns = get_time_ns() - start_time;
        return;
    }
    
    // PHASE 2: MULTI-HEAD ATTENTION (OVER ACCESSIBLE CONCEPTS ONLY)
    uint64_t attention_start = get_time_ns();
    
    // Project query once for all heads
    float query_projected[NUM_HEADS][HEAD_DIM];
    for (int h = 0; h < NUM_HEADS; h++) {
        project_embedding(query_embedding,
                        query_projected[h],
                        mha->W_query[h],
                        HEAD_DIM);
    }
    
    // Storage for head outputs
    float head_outputs[NUM_HEADS][HEAD_DIM];
    
    // Process each head
    for (int h = 0; h < NUM_HEADS; h++) {
        // Project keys and values for accessible concepts
        float keys[MAX_CONCEPTS][HEAD_DIM];
        float values[MAX_CONCEPTS][HEAD_DIM];
        
        for (int i = 0; i < accessible_count; i++) {
            project_embedding(accessible_concepts[i]->embedding,
                            keys[i],
                            mha->W_key[h],
                            HEAD_DIM);
            
            project_embedding(accessible_concepts[i]->embedding,
                            values[i],
                            mha->W_value[h],
                            HEAD_DIM);
        }
        
        // Compute attention scores (scaled dot-product)
        float scores[MAX_CONCEPTS];
        float scale = 1.0f / sqrtf((float)HEAD_DIM);
        
        for (int i = 0; i < accessible_count; i++) {
            scores[i] = 0.0f;
            for (int d = 0; d < HEAD_DIM; d++) {
                scores[i] += query_projected[h][d] * keys[i][d];
            }
            scores[i] *= scale;
        }
        
        // Softmax over accessible concepts only
        softmax(scores, accessible_count);
        
        // Store attention weights for all concepts (blocked get 0.0)
        // Initialise all to 0.0
        for (int i = 0; i < MAX_CONCEPTS; i++) {
            result->head_attention[h][i] = 0.0f;
        }
        
        // Set accessible concepts' attention weights
        for (int i = 0; i < accessible_count; i++) {
            // Find original concept index
            int orig_idx = accessible_concepts[i] - concepts;
            result->head_attention[h][orig_idx] = scores[i];
        }
        
        // Find top-k for this head
        find_topk_concepts(scores, accessible_concepts, accessible_count,
                          result->head_top_concepts[h],
                          result->head_top_weights[h], 5);
        
        // Store trace data for this head
        result->trace.head_trace[h].top_count = (accessible_count < 5) ? accessible_count : 5;
        for (int k = 0; k < result->trace.head_trace[h].top_count; k++) {
            int idx = result->head_top_concepts[h][k];
            result->trace.head_trace[h].top_concepts[k] = idx;
            result->trace.head_trace[h].top_weights[k] = result->head_top_weights[h][k];
            if (idx >= 0 && idx < concept_count) {
                memcpy(result->trace.head_trace[h].top_names[k],
                       concepts[idx].name, 63);
                result->trace.head_trace[h].top_names[k][63] = '\0';
            }
        }
        
        // Compute weighted sum of values
        memset(head_outputs[h], 0, sizeof(float) * HEAD_DIM);
        for (int i = 0; i < accessible_count; i++) {
            for (int d = 0; d < HEAD_DIM; d++) {
                head_outputs[h][d] += scores[i] * values[i][d];
            }
        }
    }
    
    result->attention_time_ns = get_time_ns() - attention_start;
    
    // PHASE 3: CONCATENATE AND PROJECT
    // Concatenate head outputs
    float concatenated[EMBEDDING_DIM];
    for (int h = 0; h < NUM_HEADS; h++) {
        for (int d = 0; d < HEAD_DIM; d++) {
            concatenated[h * HEAD_DIM + d] = head_outputs[h][d];
        }
    }
    
    // Project to output embedding
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        result->output_embedding[i] = 0.0f;
        for (int j = 0; j < EMBEDDING_DIM; j++) {
            result->output_embedding[i] += 
                mha->W_output[i][j] * concatenated[j];
        }
    }
    
    result->total_time_ns = get_time_ns() - start_time;
}

// ============================================================================
// CONCEPT DATABASE FUNCTIONS
// ============================================================================

/**
 * Helper: Create a semantically-clustered embedding
 * 
 * This creates toy embeddings that cluster by domain, mimicking
 * real semantic embeddings from Word2Vec/GloVe.
 * 
 * Strategy:
 * - Divide 64-dim space into 3 primary subspaces (medical, financial, PHI)
 * - Each domain gets a distinct subspace with high values
 * - Add variation within domain for individuality
 * - Add small noise in other subspaces for realism
 */
void create_clustered_embedding(float* embedding, int domain, int variant, int seed) {
    // Use seed for reproducibility but variation
    srand(seed);
    
    // Initialize all to small random noise
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        embedding[i] = 0.1f * ((float)rand() / RAND_MAX - 0.5f);
    }
    
    // Define subspace for each domain
    int domain_start, domain_end;
    
    if (domain == 1) {  // Medical: dimensions 0-20
        domain_start = 0;
        domain_end = 21;
    } else if (domain == 2) {  // Financial: dimensions 21-41
        domain_start = 21;
        domain_end = 42;
    } else if (domain == 3) {  // PHI: dimensions 42-63
        domain_start = 42;
        domain_end = 64;
    } else {  // Neutral: spread across all
        domain_start = 0;
        domain_end = 64;
    }
    
    // Strong signal in domain subspace
    for (int i = domain_start; i < domain_end; i++) {
        // Base signal + variant-specific pattern
        embedding[i] = 1.0f + 0.2f * variant + 
                      0.3f * sinf((float)(i - domain_start) / 3.0f + variant);
    }
    
    // Normalize to unit length (standard for semantic embeddings)
    float norm = 0.0f;
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        norm += embedding[i] * embedding[i];
    }
    norm = sqrtf(norm);
    
    if (norm > 0.0f) {
        for (int i = 0; i < EMBEDDING_DIM; i++) {
            embedding[i] /= norm;
        }
    }
}

void init_concept_database(Concept* concepts, int* count) {
    *count = 0;
    
    // Medical concepts (domain 1, prefix 0x10C5****)
    // Clustered in dimensions 0-20
    
    Concept* c = &concepts[(*count)++];
    c->tosid = 0x10C50001;
    strcpy(c->name, "aspirin");
    c->domain = 1;
    strcpy(c->tosid_full, "10C-MED-SUP-ANB:PNC-AMP-500");
    create_clustered_embedding(c->embedding, 1, 0, 1001);
    
    c = &concepts[(*count)++];
    c->tosid = 0x10C50002;
    strcpy(c->name, "ibuprofen");
    c->domain = 1;
    strcpy(c->tosid_full, "10C-MED-SUP-ANL:IBU-400-TAB");
    create_clustered_embedding(c->embedding, 1, 1, 1002);
    
    c = &concepts[(*count)++];
    c->tosid = 0x10C50003;
    strcpy(c->name, "morphine");
    c->domain = 1;
    strcpy(c->tosid_full, "10C-MED-SUP-OPI:MOR-SUL-INJ");
    create_clustered_embedding(c->embedding, 1, 2, 1003);
    
    c = &concepts[(*count)++];
    c->tosid = 0x10C50101;
    strcpy(c->name, "headache");
    c->domain = 1;
    strcpy(c->tosid_full, "10C-MED-SYM-PAI:HEA-TEN-MIG");
    create_clustered_embedding(c->embedding, 1, 3, 1004);
    
    c = &concepts[(*count)++];
    c->tosid = 0x10C50102;
    strcpy(c->name, "fever");
    c->domain = 1;
    strcpy(c->tosid_full, "10C-MED-SYM-INF:PYR-ELE-TMP");
    create_clustered_embedding(c->embedding, 1, 4, 1005);
    
    // Financial concepts (domain 2, prefix 0x11B1****)
    // Clustered in dimensions 21-41
    
    c = &concepts[(*count)++];
    c->tosid = 0x11B10001;
    strcpy(c->name, "billing_code");
    c->domain = 2;
    strcpy(c->tosid_full, "11B-FIN-BIL-COD:ICD-10-DXX");
    create_clustered_embedding(c->embedding, 2, 0, 2001);
    
    c = &concepts[(*count)++];
    c->tosid = 0x11B10002;
    strcpy(c->name, "insurance_claim");
    c->domain = 2;
    strcpy(c->tosid_full, "11B-FIN-BIL-CLM:HSP-INS-FRM");
    create_clustered_embedding(c->embedding, 2, 1, 2002);
    
    c = &concepts[(*count)++];
    c->tosid = 0x11B10003;
    strcpy(c->name, "payment_info");
    c->domain = 2;
    strcpy(c->tosid_full, "11B-FIN-BIL-PAY:CRD-INF-DAT");
    create_clustered_embedding(c->embedding, 2, 2, 2003);
    
    // PHI concepts (domain 3, prefix 0x13D2****)
    // Clustered in dimensions 42-63
    
    c = &concepts[(*count)++];
    c->tosid = 0x13D20001;
    strcpy(c->name, "patient_ssn");
    c->domain = 3;
    strcpy(c->tosid_full, "11B-PER-IDN-GOV:USA-SSN-NUM");
    create_clustered_embedding(c->embedding, 3, 0, 3001);
    
    c = &concepts[(*count)++];
    c->tosid = 0x13D20002;
    strcpy(c->name, "patient_name");
    c->domain = 3;
    strcpy(c->tosid_full, "11B-PER-IDN-NAM:PAT-REC-FUL");
    create_clustered_embedding(c->embedding, 3, 1, 3002);
    
    printf("Initialised %d concepts (semantically clustered):\n", *count);
    printf("  Medical: 5 concepts (dimensions 0-20, domain 1)\n");
    printf("  Financial: 3 concepts (dimensions 21-41, domain 2)\n");
    printf("  PHI: 2 concepts (dimensions 42-63, domain 3)\n");
}

void print_concept(const Concept* concept) {
    printf("Concept: %s\n", concept->name);
    printf("  TOSID (POC):  0x%08X\n", concept->tosid);
    if (strlen(concept->tosid_full) > 0) {
        printf("  TOSID (Full): %s\n", concept->tosid_full);
    }
    printf("  Domain: %d\n", concept->domain);
    printf("  Embedding: [%.3f, %.3f, ... %.3f]\n",
           concept->embedding[0],
           concept->embedding[1],
           concept->embedding[EMBEDDING_DIM-1]);
}

// ============================================================================
// REPORTING FUNCTIONS (Call after computation for clean timing)
// ============================================================================

void print_attention_result(const MultiHeadAttentionResult* result,
                            const Concept* concepts,
                            int verbose) {
    (void)concepts;  // Unused parameter (data already in result->trace)
    
    // PHASE 1: Routing Enforcement Results
    printf("\n========================================\n");
    printf("PHASE 1: Routing Enforcement\n");
    printf("  Context: %s\n", result->routing_context_name);
    printf("========================================\n");
    
    if (verbose) {
        for (int i = 0; i < result->trace.routing_trace.count; i++) {
            if (result->trace.routing_trace.accessible[i]) {
                printf("  " ANSI_GREEN "✓ ALLOW" ANSI_RESET ": %-20s [0x%08X]", 
                       result->trace.routing_trace.names[i],
                       result->trace.routing_trace.tosids[i]);
                if (strlen(result->trace.routing_trace.tosid_full[i]) > 0) {
                    printf(" → %s", result->trace.routing_trace.tosid_full[i]);
                }
                printf("\n");
            } else {
                printf("  " ANSI_RED "✗ BLOCK" ANSI_RESET ": %-20s [0x%08X]", 
                       result->trace.routing_trace.names[i],
                       result->trace.routing_trace.tosids[i]);
                if (strlen(result->trace.routing_trace.tosid_full[i]) > 0) {
                    printf(" → %s", result->trace.routing_trace.tosid_full[i]);
                }
                printf("\n");
            }
        }
    }
    
    printf("\n" ANSI_CYAN "Routing Summary:" ANSI_RESET "\n");
    printf("  Total concepts: %d\n", result->trace.routing_trace.count);
    printf("  Accessible: " ANSI_GREEN "%d" ANSI_RESET "\n", result->total_accessible_concepts);
    printf("  Blocked: " ANSI_RED "%d" ANSI_RESET "\n", result->total_blocked_concepts);
    printf("  Routing time: %" PRIu64 " ns (%.3f μs)\n", 
           result->routing_time_ns, result->routing_time_ns / 1000.0);
    
    if (result->total_accessible_concepts == 0) {
        printf("\n" ANSI_RED "ERROR: No accessible concepts! Cannot proceed." ANSI_RESET "\n");
        return;
    }
    
    // PHASE 2: Multi-Head Attention Results
    if (verbose) {
        printf("\n========================================\n");
        printf("PHASE 2: Multi-Head Attention\n");
        printf("========================================\n");
        
        for (int h = 0; h < NUM_HEADS; h++) {
            printf("\n" ANSI_YELLOW "--- Head %d ---" ANSI_RESET "\n", h + 1);
            printf("Top attended concepts:\n");
            
            for (int k = 0; k < result->trace.head_trace[h].top_count; k++) {
                printf("  %d. %-20s (%.1f%%)\n",
                       k + 1,
                       result->trace.head_trace[h].top_names[k],
                       result->trace.head_trace[h].top_weights[k] * 100.0f);
            }
        }
        
        printf("\n========================================\n");
        printf("PHASE 3: Output Projection\n");
        printf("========================================\n");
    }
    
    // Performance Summary
    printf("\n" ANSI_CYAN "Performance Summary:" ANSI_RESET "\n");
    printf("  Routing time:   %6" PRIu64 " ns (%6.3f μs)\n", 
           result->routing_time_ns, result->routing_time_ns / 1000.0);
    printf("  Attention time: %6" PRIu64 " ns (%6.3f μs)\n", 
           result->attention_time_ns, result->attention_time_ns / 1000.0);
    printf("  Total time:     %6" PRIu64 " ns (%6.3f μs)\n", 
           result->total_time_ns, result->total_time_ns / 1000.0);
    printf("  Routing overhead: %.1f%%\n",
           100.0 * result->routing_time_ns / (double)result->total_time_ns);
}
