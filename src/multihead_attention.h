/*
 * multihead_attention.h
 * 
 * Multi-Head Attention with TOSID-based Routing Integration
 * CARM Proof-of-Concept Implementation
 * 
 * TOSID Format Note:
 * This POC uses simplified 32-bit hexadecimal TOSID format for implementation
 * efficiency whilst preserving the routing mechanism. The simplified format is
 * directly compatible with full TOSID format - only the parsing layer differs.
 * 
 * Simplified Format: 0xTTNXXXXX
 *   TT:    Taxonomy code (8 bits)
 *   N:     Scope indicator (4 bits)
 *   XXXXX: Entity identifier (20 bits)
 * 
 * Full TOSID Format: TTN-XXX-XXX-XXX:XXX-XXX-XXX-XXX
 * Example: 10C-MED-SUP-ANB:PNC-AMP-500 (Medical antibiotic)
 * 
 * See docs/ADR-001-TOSID-Simplified-Format.md for complete rationale.
 */
/*
 * multihead_attention.h
 *
 * CARM -- Controlled Attention Routing and Masking
 * Multi-head attention with CARM enforcement: types and API
 *
 * Copyright (c) 2026 haitch
 * Licensed under the Apache License, Version 2.0
 * https://www.apache.org/licenses/LICENSE-2.0
 */


#ifndef MULTIHEAD_ATTENTION_H
#define MULTIHEAD_ATTENTION_H

#include <stdint.h>
#include <time.h>
#include "routing_table.h"  // O(1) routing lookup backend

// ============================================================================
// CONFIGURATION CONSTANTS
// ============================================================================

#define EMBEDDING_DIM 64
#define NUM_HEADS 8
#define HEAD_DIM (EMBEDDING_DIM / NUM_HEADS)  // 8
#define MAX_CONCEPTS 100
#define MAX_RULES 10

// ============================================================================
// TOSID DEFINITIONS (SIMPLIFIED FORMAT)
// ============================================================================

// TOSID type (32-bit hexadecimal)
typedef uint32_t TOSID;

// TOSID manipulation macros
#define TOSID_GET_TAXONOMY(tosid)    (((tosid) >> 24) & 0xFF)
#define TOSID_GET_SCOPE(tosid)       (((tosid) >> 20) & 0x0F)
/* TOSID_GET_DOMAIN_CATEGORY: extracts domain+category prefix (bits 23-16).
 * Use TOSID_GET_DOMAIN from routing_table.h for the domain byte alone. */
#define TOSID_GET_DOMAIN_CATEGORY(tosid)  (((tosid) >> 16) & 0xFFFF)
#define TOSID_GET_ENTITY(tosid)      ((tosid) & 0xFFFFF)

// TOSID constructor macro
#define MAKE_TOSID(taxonomy, scope, entity) \
    ((((uint32_t)(taxonomy)) << 24) | \
     (((uint32_t)(scope)) << 20) | \
     ((uint32_t)(entity)))

// Domain prefixes (simplified format maps to full TOSID)
// Medical:   0x10C5**** → 10C-MED-***
#define TOSID_MEDICAL_PREFIX     0x10C50000
#define TOSID_MEDICAL_MASK       0xFFFF0000

// Financial: 0x11B1**** → 11B-FIN-***
#define TOSID_FINANCIAL_PREFIX   0x11B10000
#define TOSID_FINANCIAL_MASK     0xFFFF0000

// PHI:       0x13D2**** → 11B-PER-IDN-***
#define TOSID_PHI_PREFIX         0x13D20000
#define TOSID_PHI_MASK           0xFFFF0000

// Neutral:   0x00****** → Various neutral concepts
#define TOSID_NEUTRAL_PREFIX     0x00000000
#define TOSID_NEUTRAL_MASK       0xFF000000

// ============================================================================
// CONCEPT REPRESENTATION
// ============================================================================

typedef struct {
    TOSID tosid;                      // Simplified TOSID (32-bit hex)
    char name[64];                    // Human-readable name
    float embedding[EMBEDDING_DIM];   // Semantic embedding
    int domain;                       // 0=neutral, 1=medical, 2=financial, 3=PHI
    
    // Optional: Full TOSID string for future migration
    char tosid_full[64];              // e.g., "10C-MED-SUP-ANB:PNC-AMP-500"
} Concept;

// ============================================================================
// ROUTING TABLE (KMI LAYER)
// ============================================================================

// Routing action
typedef enum {
    ALLOW = 1,
    DENY = 0
} RouteAction;

// Routing rule
typedef struct {
    TOSID prefix;           // TOSID prefix to match (e.g., 0x10C50000)
    TOSID mask;             // Prefix mask (e.g., 0xFFFF0000)
    RouteAction action;     // ALLOW or DENY
    char description[128];  // Human-readable description
    
    // Optional: Full TOSID pattern for documentation
    char tosid_pattern[64]; // e.g., "10C-MED-***"
} RoutingRule;

// Routing table
typedef struct {
    RoutingRule rules[MAX_RULES];
    int rule_count;
    char context_name[64];  // e.g., "Clinical Context"
    
    // O(1) routing lookup backend (64 KB)
    // This is built from rules for fast lookups
    FastRoutingTable fast_lookup;  // from routing_table.h
} RoutingTable;

// ============================================================================
// MULTI-HEAD ATTENTION
// ============================================================================

// Multi-head attention weight matrices
typedef struct {
    // Per-head projection matrices
    float W_query[NUM_HEADS][HEAD_DIM][EMBEDDING_DIM];
    float W_key[NUM_HEADS][HEAD_DIM][EMBEDDING_DIM];
    float W_value[NUM_HEADS][HEAD_DIM][EMBEDDING_DIM];
    
    // Shared output projection
    float W_output[EMBEDDING_DIM][EMBEDDING_DIM];
    
    // Metadata
    int num_heads;
    int head_dim;
    int embedding_dim;
} MultiHeadAttention;

// Trace data for reporting (collected during computation)
typedef struct {
    // Routing phase trace
    struct {
        TOSID tosids[MAX_CONCEPTS];
        char names[MAX_CONCEPTS][64];
        char tosid_full[MAX_CONCEPTS][64];
        int accessible[MAX_CONCEPTS];  // 1 = allow, 0 = block
        int count;
    } routing_trace;
    
    // Per-head trace
    struct {
        int top_concepts[5];
        float top_weights[5];
        char top_names[5][64];
        int top_count;
    } head_trace[NUM_HEADS];
} AttentionTraceData;

// Multi-head attention result
typedef struct {
    // Overall results
    float output_embedding[EMBEDDING_DIM];
    int total_accessible_concepts;
    int total_blocked_concepts;
    
    // Per-head attention weights [head][concept_idx]
    float head_attention[NUM_HEADS][MAX_CONCEPTS];
    
    // Per-head top-k attended concepts (for analysis)
    int head_top_concepts[NUM_HEADS][5];
    float head_top_weights[NUM_HEADS][5];
    
    // Routing enforcement results
    TOSID blocked_tosids[MAX_CONCEPTS];
    int num_blocked;
    
    // Performance metrics
    uint64_t routing_time_ns;
    uint64_t attention_time_ns;
    uint64_t total_time_ns;
    
    // Trace data (for reporting after computation)
    AttentionTraceData trace;
    char routing_context_name[64];
} MultiHeadAttentionResult;

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

// --- Routing Table Functions ---

/**
 * Check if TOSID is accessible according to routing table
 * Returns 1 if accessible, 0 if blocked
 */
int check_routing_access(TOSID tosid, RoutingTable* table);

/**
 * Build O(1) fast lookup table from routing rules
 * This should be called after adding rules to enable fast routing
 */
void build_fast_routing_table(RoutingTable* table);

/**
 * Initialise clinical routing context
 * Allows: Medical concepts (0x10C5****)
 * Blocks: Financial (0x11B1****), PHI (0x13D2****)
 */
void init_routing_clinical(RoutingTable* table);

/**
 * Initialise unrestricted routing context
 * Allows: All concepts
 */
void init_routing_unrestricted(RoutingTable* table);

/**
 * Initialise deny-all routing context (for edge case testing)
 */
void init_routing_deny_all(RoutingTable* table);

// --- Multi-Head Attention Functions ---

/**
 * Initialise multi-head attention with Xavier initialisation
 */
void init_multihead_attention(MultiHeadAttention* mha);

/**
 * Compute multi-head attention with routing enforcement
 * 
 * This is the core function implementing:
 * 1. Routing filter (once for all heads)
 * 2. Per-head attention (over accessible concepts only)
 * 3. Concatenation and output projection
 * 
 * Key property: Blocked concepts receive ZERO attention in ALL heads
 * 
 * Note: This function focuses on computation only. Use print_attention_result()
 * to display the results after computation for accurate performance measurement.
 */
void compute_multihead_attention_with_routing(
    float* query_embedding,
    Concept* concepts,
    int concept_count,
    RoutingTable* routing_table,
    MultiHeadAttention* mha,
    MultiHeadAttentionResult* result
);

/**
 * Print attention computation results (call after computation for clean timing)
 * 
 * This separates reporting from computation to ensure performance measurements
 * are not affected by console I/O overhead.
 */
void print_attention_result(const MultiHeadAttentionResult* result,
                            const Concept* concepts,
                            int verbose);

// --- Helper Functions ---

/**
 * Project embedding through weight matrix
 * output[i] = Σ_j (weights[i][j] * input[j])
 */
void project_embedding(const float* input, 
                      float* output,
                      float weights[][EMBEDDING_DIM],
                      int output_dim);

/**
 * Softmax function with numerical stability
 */
void softmax(float* scores, int count);

/**
 * Find top-k concepts by attention weight
 */
void find_topk_concepts(float* scores,
                       Concept** concepts,
                       int count,
                       int* top_indices,
                       float* top_weights,
                       int k);

/**
 * Generate random value from normal distribution (Box-Muller)
 */
float random_normal(float mean, float stddev);

/**
 * Get high-resolution timestamp in nanoseconds
 */
uint64_t get_time_ns(void);

/**
 * Compute cosine similarity between two embeddings
 */
float cosine_similarity(const float* a, const float* b, int dim);

// --- Concept Database Functions ---

/**
 * Initialise concept database with test data
 * Creates mixed medical, financial, PHI, and neutral concepts
 */
void init_concept_database(Concept* concepts, int* count);

/**
 * Print concept for debugging
 */
void print_concept(const Concept* concept);

// ============================================================================
// UTILITY MACROS
// ============================================================================

// ANSI colour codes for terminal output
#define ANSI_RED     "\x1b[31m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN    "\x1b[36m"
#define ANSI_RESET   "\x1b[0m"

// Assertion with message
#define ASSERT_MSG(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "Assertion failed: %s\n  File: %s, Line: %d\n  Message: %s\n", \
                    #condition, __FILE__, __LINE__, message); \
            exit(1); \
        } \
    } while (0)

#endif // MULTIHEAD_ATTENTION_H
