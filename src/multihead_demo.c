/*
 * multihead_demo.c
 * 
 * Demonstration of Multi-Head Attention with TOSID-based Routing
 * Shows visual comparison between different routing contexts
 * 
 * Updated: Uses separated reporting for clean performance measurement
 */
/*
 * multihead_demo.c
 *
 * CARM -- Controlled Attention Routing and Masking
 * Interactive demonstration of CARM routing in two contexts
 *
 * Copyright (c) 2026 haitch
 * Licensed under the Apache License, Version 2.0
 * https://www.apache.org/licenses/LICENSE-2.0
 */


#include "multihead_attention.h"
#include "carm_version.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ============================================================================
// DEMONSTRATION SCENARIOS
// ============================================================================

void demo_clinical_context() {
    printf("\n");
    printf(ANSI_CYAN "╔════════════════════════════════════════════════════════════╗\n");
    printf("║  DEMONSTRATION: Clinical Documentation Context            ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n" ANSI_RESET);
    
    // Setup
    Concept concepts[MAX_CONCEPTS];
    int concept_count;
    init_concept_database(concepts, &concept_count);
    
    RoutingTable routing;
    init_routing_clinical(&routing);
    
    MultiHeadAttention mha;
    init_multihead_attention(&mha);
    
    // Medical query (similar to symptom "headache")
    printf("\nQuery: Medical symptom similar to 'headache'\n");
    float query[EMBEDDING_DIM];
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        query[i] = 0.05f * ((float)rand() / RAND_MAX - 0.5f);
    }
    for (int i = 0; i < 21; i++) {
        query[i] = 1.0f + 0.3f * sinf((float)i / 3.0f);
    }
    float norm = 0.0f;
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        norm += query[i] * query[i];
    }
    norm = sqrtf(norm);
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        query[i] /= norm;
    }
    
    // Compute attention (clean timing - no I/O during computation)
    MultiHeadAttentionResult result;
    compute_multihead_attention_with_routing(
        query, concepts, concept_count, &routing, &mha, &result);
    
    // Print results after computation
    print_attention_result(&result, concepts, 1);
    
    // Summary
    printf("\n" ANSI_GREEN "Key Findings:" ANSI_RESET "\n");
    printf("  ✓ Medical concepts (aspirin, ibuprofen, etc.) accessible\n");
    printf("  ✗ Financial concepts (billing, insurance) blocked\n");
    printf("  ✗ PHI (patient SSN, name) blocked\n");
    printf("\n  Result: System can discuss medical treatments\n");
    printf("          but CANNOT mention billing or patient identifiers\n");
}

void demo_unrestricted_context() {
    printf("\n");
    printf(ANSI_CYAN "╔════════════════════════════════════════════════════════════╗\n");
    printf("║  DEMONSTRATION: Unrestricted Context                      ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n" ANSI_RESET);
    
    // Setup
    Concept concepts[MAX_CONCEPTS];
    int concept_count;
    init_concept_database(concepts, &concept_count);
    
    RoutingTable routing;
    init_routing_unrestricted(&routing);
    
    MultiHeadAttention mha;
    init_multihead_attention(&mha);
    
    // Same medical query
    printf("\nQuery: Same medical symptom query\n");
    float query[EMBEDDING_DIM];
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        query[i] = 0.05f * ((float)rand() / RAND_MAX - 0.5f);
    }
    for (int i = 0; i < 21; i++) {
        query[i] = 1.0f + 0.3f * sinf((float)i / 3.0f);
    }
    float norm = 0.0f;
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        norm += query[i] * query[i];
    }
    norm = sqrtf(norm);
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        query[i] /= norm;
    }
    
    // Compute attention
    MultiHeadAttentionResult result;
    compute_multihead_attention_with_routing(
        query, concepts, concept_count, &routing, &mha, &result);
    
    // Print results
    print_attention_result(&result, concepts, 1);
    
    // Summary
    printf("\n" ANSI_GREEN "Key Findings:" ANSI_RESET "\n");
    printf("  ✓ Medical concepts accessible\n");
    printf("  ✓ Financial concepts accessible\n");
    printf("  ✓ PHI accessible\n");
    printf("\n  Result: System has access to ALL concepts\n");
    printf("          including billing and patient identifiers\n");
}

void demo_comparison() {
    printf("\n");
    printf(ANSI_CYAN "╔════════════════════════════════════════════════════════════╗\n");
    printf("║  DEMONSTRATION: Side-by-Side Comparison                   ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n" ANSI_RESET);
    
    // Setup
    Concept concepts[MAX_CONCEPTS];
    int concept_count;
    init_concept_database(concepts, &concept_count);
    
    MultiHeadAttention mha;
    init_multihead_attention(&mha);
    
    // Financial query (similar to "billing")
    printf("\nQuery: Similar to 'billing_code' (financial concept)\n");
    float query[EMBEDDING_DIM];
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        query[i] = 0.05f * ((float)rand() / RAND_MAX - 0.5f);
    }
    // Strong signal in financial dimensions (21-41)
    for (int i = 21; i < 42; i++) {
        query[i] = 1.0f + 0.2f * sinf((float)(i - 21) / 3.0f);
    }
    float norm = 0.0f;
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        norm += query[i] * query[i];
    }
    norm = sqrtf(norm);
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        query[i] /= norm;
    }
    
    // Clinical context (blocks financial)
    printf("\n" ANSI_YELLOW "--- Clinical Context (Financial BLOCKED) ---" ANSI_RESET "\n");
    RoutingTable routing_clinical;
    init_routing_clinical(&routing_clinical);
    
    MultiHeadAttentionResult result_clinical;
    compute_multihead_attention_with_routing(
        query, concepts, concept_count, &routing_clinical, &mha, &result_clinical);
    
    print_attention_result(&result_clinical, concepts, 0);
    
    // Unrestricted context (allows financial)
    printf("\n" ANSI_YELLOW "--- Unrestricted Context (Financial ALLOWED) ---" ANSI_RESET "\n");
    RoutingTable routing_unrestricted;
    init_routing_unrestricted(&routing_unrestricted);
    
    MultiHeadAttentionResult result_unrestricted;
    compute_multihead_attention_with_routing(
        query, concepts, concept_count, &routing_unrestricted, &mha, &result_unrestricted);
    
    print_attention_result(&result_unrestricted, concepts, 0);
    
    // Compare outputs
    float similarity = cosine_similarity(
        result_clinical.output_embedding,
        result_unrestricted.output_embedding,
        EMBEDDING_DIM);
    
    printf("\n" ANSI_CYAN "Comparison:" ANSI_RESET "\n");
    printf("  Output similarity: %.3f\n", similarity);
    printf("\n");
    
    // Find billing concept
    int billing_idx = -1;
    for (int i = 0; i < concept_count; i++) {
        if (strcmp(concepts[i].name, "billing_code") == 0) {
            billing_idx = i;
            break;
        }
    }
    
    if (billing_idx >= 0) {
        // Count billing appearances in top-k
        int billing_in_clinical = 0;
        int billing_in_unrestricted = 0;
        
        for (int h = 0; h < NUM_HEADS; h++) {
            for (int k = 0; k < 5; k++) {
                if (result_clinical.head_top_concepts[h][k] == billing_idx) {
                    billing_in_clinical++;
                }
                if (result_unrestricted.head_top_concepts[h][k] == billing_idx) {
                    billing_in_unrestricted++;
                }
            }
        }
        
        printf("  'billing_code' in top-k:\n");
        printf("    Clinical context:     %d times (out of 40 possible)\n", 
               billing_in_clinical);
        printf("    Unrestricted context: %d times (out of 40 possible)\n",
               billing_in_unrestricted);
    }
    
    printf("\n" ANSI_GREEN "Key Insight:" ANSI_RESET "\n");
    printf("  Even with a query SIMILAR to 'billing', the clinical\n");
    printf("  context prevents the system from attending to financial\n");
    printf("  concepts. This is architectural, not behavioral.\n");
}

void demo_head_visualization() {
    printf("\n");
    printf(ANSI_CYAN "╔════════════════════════════════════════════════════════════╗\n");
    printf("║  DEMONSTRATION: Head Attention Visualization              ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n" ANSI_RESET);
    
    // Setup
    Concept concepts[MAX_CONCEPTS];
    int concept_count;
    init_concept_database(concepts, &concept_count);
    
    RoutingTable routing;
    init_routing_clinical(&routing);
    
    MultiHeadAttention mha;
    init_multihead_attention(&mha);
    
    // Medical query
    float query[EMBEDDING_DIM];
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        query[i] = 0.05f * ((float)rand() / RAND_MAX - 0.5f);
    }
    for (int i = 0; i < 21; i++) {
        query[i] = 1.0f + 0.3f * sinf((float)i / 3.0f);
    }
    float norm = 0.0f;
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        norm += query[i] * query[i];
    }
    norm = sqrtf(norm);
    for (int i = 0; i < EMBEDDING_DIM; i++) {
        query[i] /= norm;
    }
    
    MultiHeadAttentionResult result;
    compute_multihead_attention_with_routing(
        query, concepts, concept_count, &routing, &mha, &result);
    
    // Print compact summary
    printf("\nRouting: %s\n", result.routing_context_name);
    printf("Accessible: %d concepts | Blocked: %d concepts\n",
           result.total_accessible_concepts, result.total_blocked_concepts);
    printf("Routing time: %.2f μs | Total time: %.2f μs\n\n",
           result.routing_time_ns / 1000.0, result.total_time_ns / 1000.0);
    
    // Visualize attention across heads
    printf("Attention Distribution Across Heads:\n\n");
    
    // Get accessible concepts
    int accessible_indices[MAX_CONCEPTS];
    int accessible_count = 0;
    for (int i = 0; i < concept_count; i++) {
        if (check_routing_access(concepts[i].tosid, &routing)) {
            accessible_indices[accessible_count++] = i;
        }
    }
    
    // Print header
    printf("%-15s", "Concept");
    for (int h = 0; h < NUM_HEADS; h++) {
        printf(" H%d  ", h + 1);
    }
    printf("\n");
    printf("%-15s", "-------");
    for (int h = 0; h < NUM_HEADS; h++) {
        printf(" --- ");
    }
    printf("\n");
    
    // Print attention for each accessible concept
    for (int i = 0; i < accessible_count; i++) {
        int idx = accessible_indices[i];
        printf("%-15s", concepts[idx].name);
        
        for (int h = 0; h < NUM_HEADS; h++) {
            float weight = result.head_attention[h][idx];
            
            // Visual bar chart
            if (weight < 0.05f) {
                printf(" .   ");
            } else if (weight < 0.15f) {
                printf(" ▁   ");
            } else if (weight < 0.25f) {
                printf(" ▃   ");
            } else if (weight < 0.35f) {
                printf(" ▅   ");
            } else {
                printf(" ▇   ");
            }
        }
        printf("\n");
    }
    
    printf("\nLegend: . (<5%%)  ▁ (5-15%%)  ▃ (15-25%%)  ▅ (25-35%%)  ▇ (>35%%)\n");
    
    printf("\n" ANSI_GREEN "Observation:" ANSI_RESET "\n");
    printf("  Different heads attend to different concepts,\n");
    printf("  demonstrating specialization within the multi-head\n");
    printf("  architecture while respecting routing constraints.\n");
}

// ============================================================================
// MAIN DEMO
// ============================================================================

int main(int argc, char** argv) {
    (void)argc;  // Suppress unused warning
    (void)argv;
    
    // Seed random number generator
    srand(42);
    
    printf("\n");
    printf(ANSI_CYAN "════════════════════════════════════════════════════════════\n");
    printf("  Multi-Head Attention with Routing - Interactive Demo\n");
    carm_version_print();
    printf("  Controlled Attention Routing and Masking\n");
    printf("════════════════════════════════════════════════════════════\n" ANSI_RESET);
    
    // Run demonstrations
    demo_clinical_context();
    
    printf("\n\nPress Enter to continue...");
    getchar();
    
    demo_unrestricted_context();
    
    printf("\n\nPress Enter to continue...");
    getchar();
    
    demo_comparison();
    
    printf("\n\nPress Enter to continue...");
    getchar();
    
    demo_head_visualization();
    
    printf("\n");
    printf(ANSI_CYAN "════════════════════════════════════════════════════════════\n");
    printf("  End of Demonstration\n");
    printf("════════════════════════════════════════════════════════════\n" ANSI_RESET);
    printf("\n");
    
    return 0;
}
