/*
 * embeddings.h
 * 
 * CARM Phase 2 - GloVe Embeddings at Scale
 * Binary embedding loader for GloVe/Word2Vec pre-trained embeddings
 */
/*
 * embeddings.h
 *
 * CARM -- Controlled Attention Routing and Masking
 * Binary embedding loader: API for GloVe/Word2Vec concept vectors
 *
 * Copyright (c) 2026 haitch
 * Licensed under the Apache License, Version 2.0
 * https://www.apache.org/licenses/LICENSE-2.0
 */


#ifndef EMBEDDINGS_H
#define EMBEDDINGS_H

#include <stdint.h>
#include "multihead_attention.h"

// ============================================================================
// BINARY FORMAT SPECIFICATION
// ============================================================================

/*
 * Binary Embedding File Format (Version 1)
 * 
 * Header (16 bytes):
 *   [version:4]  - Format version (currently 1)
 *   [count:4]    - Number of concepts in file
 *   [dim:4]      - Embedding dimensionality
 *   [reserved:4] - Reserved for future use
 * 
 * Concepts (repeated 'count' times):
 *   [tosid:4]      - TOSID hex value (uint32_t)
 *   [name:64]      - Concept name (null-padded string)
 *   [embedding:dim*4] - Embedding vector (float32)
 * 
 * Total size: 16 + count * (4 + 64 + dim*4) bytes
 * Example: 1000 concepts @ 100-dim = 16 + 1000*(68+400) = 468,016 bytes (~457 KB)
 */

typedef struct {
    uint32_t version;
    uint32_t count;
    uint32_t dim;
    uint32_t reserved;
} EmbeddingHeader;

// ============================================================================
// LOADER FUNCTIONS
// ============================================================================

/**
 * Load embeddings from binary file into ConceptDatabase
 * 
 * This function:
 * 1. Opens and validates binary file
 * 2. Checks version and dimension compatibility
 * 3. Reads concepts and embeddings
 * 4. Computes embedding norms
 * 5. Infers domains from TOSID prefixes
 * 
 * Args:
 *   filename: Path to binary embedding file (e.g., "embeddings_glove_100.bin")
 *   db: Pointer to ConceptDatabase to populate
 * 
 * Returns:
 *   0 on success
 *   -1 on error (with stderr message)
 * 
 * Example:
 *   ConceptDatabase db;
 *   if (load_embeddings_binary("embeddings/embeddings_glove_100.bin", &db) == 0) {
 *       printf("Loaded %d concepts with %d-dim embeddings\n",
 *              db.count, db.embedding_dim);
 *   }
 */
int load_embeddings_binary(const char* filename, ConceptDatabase* db);

/**
 * Validate embedding file format
 * 
 * Checks:
 * - File exists and is readable
 * - Version is supported
 * - Dimension matches EMBEDDING_DIM
 * - Count is within MAX_CONCEPTS
 * 
 * Args:
 *   filename: Path to binary file
 *   header: Pointer to header struct to populate (can be NULL)
 * 
 * Returns:
 *   0 if valid
 *   -1 if invalid (with stderr message)
 */
int validate_embedding_file(const char* filename, EmbeddingHeader* header);

/**
 * Print embedding file info
 * 
 * Displays:
 * - Format version
 * - Concept count
 * - Embedding dimension
 * - File size
 * - First 10 concept names
 */
void print_embedding_info(const char* filename);

/**
 * Print concept database statistics
 * 
 * Shows:
 * - Total concepts
 * - Domain breakdown (medical, financial, PHI, neutral)
 * - Embedding dimension
 * - Memory usage
 * - Source information
 */
void print_concept_database_info(const ConceptDatabase* db);

#endif // EMBEDDINGS_H
