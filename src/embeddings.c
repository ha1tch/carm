/*
 * embeddings.c
 * 
 * CARM Phase 2 - GloVe Embeddings at Scale
 * Binary embedding loader implementation
 */
/*
 * embeddings.c
 *
 * CARM -- Controlled Attention Routing and Masking
 * Binary embedding loader implementation
 *
 * Copyright (c) 2026 haitch
 * Licensed under the Apache License, Version 2.0
 * https://www.apache.org/licenses/LICENSE-2.0
 */


#include "embeddings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int validate_embedding_file(const char* filename, EmbeddingHeader* header) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Error: Cannot open %s\n", filename);
        return -1;
    }
    
    // Read header
    EmbeddingHeader h;
    size_t bytes_read = fread(&h, sizeof(EmbeddingHeader), 1, f);
    if (bytes_read != 1) {
        fprintf(stderr, "Error: Cannot read header from %s\n", filename);
        fclose(f);
        return -1;
    }
    
    // Validate version
    if (h.version != 1) {
        fprintf(stderr, "Error: Unsupported version %d (expected 1)\n", h.version);
        fclose(f);
        return -1;
    }
    
    // Validate dimension
    if (h.dim != EMBEDDING_DIM) {
        fprintf(stderr, "Error: Dimension mismatch (file: %d, compiled: %d)\n",
                h.dim, EMBEDDING_DIM);
        fprintf(stderr, "       Recompile with EMBEDDING_DIM=%d or use different embedding file\n",
                h.dim);
        fclose(f);
        return -1;
    }
    
    // Validate count
    if (h.count > MAX_CONCEPTS) {
        fprintf(stderr, "Error: Too many concepts (%d > %d)\n",
                h.count, MAX_CONCEPTS);
        fprintf(stderr, "       Recompile with MAX_CONCEPTS=%d or reduce vocabulary\n",
                h.count);
        fclose(f);
        return -1;
    }
    
    if (h.count == 0) {
        fprintf(stderr, "Error: Empty embedding file (count = 0)\n");
        fclose(f);
        return -1;
    }
    
    fclose(f);
    
    // Copy header if requested
    if (header != NULL) {
        *header = h;
    }
    
    return 0;
}

int load_embeddings_binary(const char* filename, ConceptDatabase* db) {
    // Validate file first
    EmbeddingHeader header;
    if (validate_embedding_file(filename, &header) != 0) {
        return -1;
    }
    
    // Open file
    FILE* f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Error: Cannot open %s\n", filename);
        return -1;
    }
    
    // Skip header (already validated)
    fseek(f, sizeof(EmbeddingHeader), SEEK_SET);
    
    // Initialize database
    db->count = header.count;
    db->embedding_dim = header.dim;
    snprintf(db->source, sizeof(db->source), "Binary file: %s", filename);
    snprintf(db->build_date, sizeof(db->build_date), "Unknown");
    
    // Read concepts
    for (uint32_t i = 0; i < header.count; i++) {
        Concept* c = &db->concepts[i];
        
        // Read TOSID
        if (fread(&c->tosid, sizeof(uint32_t), 1, f) != 1) {
            fprintf(stderr, "Error: Failed to read TOSID for concept %d\n", i);
            fclose(f);
            return -1;
        }
        
        // Read name
        if (fread(c->name, 64, 1, f) != 1) {
            fprintf(stderr, "Error: Failed to read name for concept %d\n", i);
            fclose(f);
            return -1;
        }
        c->name[63] = '\0';  // Ensure null termination
        
        // Read embedding
        if (fread(c->embedding, sizeof(float), header.dim, f) != header.dim) {
            fprintf(stderr, "Error: Failed to read embedding for concept %d (%s)\n",
                    i, c->name);
            fclose(f);
            return -1;
        }
        
        // Compute embedding norm
        float norm_sq = 0.0f;
        for (int d = 0; d < header.dim; d++) {
            norm_sq += c->embedding[d] * c->embedding[d];
        }
        c->embedding_norm = sqrtf(norm_sq);
        
        // Infer domain from TOSID
        uint8_t domain_byte = (c->tosid >> 24) & 0xFF;
        if (domain_byte == 0x10) {
            c->domain = 1;  // Medical
            strcpy(c->domain_name, "medical");
        } else if (domain_byte == 0x11) {
            c->domain = 2;  // Financial
            strcpy(c->domain_name, "financial");
        } else if (domain_byte == 0x13) {
            c->domain = 3;  // PHI
            strcpy(c->domain_name, "phi");
        } else {
            c->domain = 0;  // Neutral
            strcpy(c->domain_name, "neutral");
        }
        
        // Copy TOSID to full string (simplified representation)
        snprintf(c->tosid_full, sizeof(c->tosid_full), "0x%08X", c->tosid);
        
        // Set source
        strcpy(c->source, "GloVe-100");  // TODO: Extract from metadata
        
        // Frequency unknown
        c->frequency = 0;
    }
    
    fclose(f);
    
    printf("Loaded %d concepts with %d-dimensional embeddings\n",
           db->count, db->embedding_dim);
    
    return 0;
}

void print_embedding_info(const char* filename) {
    EmbeddingHeader header;
    if (validate_embedding_file(filename, &header) != 0) {
        return;
    }
    
    FILE* f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Error: Cannot open %s\n", filename);
        return;
    }
    
    // Get file size
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, sizeof(EmbeddingHeader), SEEK_SET);
    
    printf("Embedding File Info: %s\n", filename);
    printf("="*60);
    printf("\n");
    printf("  Version:    %d\n", header.version);
    printf("  Concepts:   %d\n", header.count);
    printf("  Dimensions: %d\n", header.dim);
    printf("  File size:  %.2f MB\n", file_size / (1024.0 * 1024.0));
    printf("\n");
    printf("First 10 concepts:\n");
    
    // Read and print first 10 concepts
    for (int i = 0; i < 10 && i < (int)header.count; i++) {
        uint32_t tosid;
        char name[64];
        
        fread(&tosid, sizeof(uint32_t), 1, f);
        fread(name, 64, 1, f);
        name[63] = '\0';
        
        // Skip embedding
        fseek(f, header.dim * sizeof(float), SEEK_CUR);
        
        printf("  %2d. %-20s [0x%08X]\n", i+1, name, tosid);
    }
    
    fclose(f);
}

void print_concept_database_info(const ConceptDatabase* db) {
    printf("Concept Database Info:\n");
    printf("="*60);
    printf("\n");
    printf("  Total concepts:      %d\n", db->count);
    printf("  Embedding dimension: %d\n", db->embedding_dim);
    printf("  Source:              %s\n", db->source);
    printf("\n");
    
    // Count by domain
    int medical = 0, financial = 0, phi = 0, neutral = 0;
    for (int i = 0; i < db->count; i++) {
        switch (db->concepts[i].domain) {
            case 1: medical++; break;
            case 2: financial++; break;
            case 3: phi++; break;
            default: neutral++; break;
        }
    }
    
    printf("Domain breakdown:\n");
    printf("  Medical:   %4d (%5.1f%%)\n", medical, 100.0*medical/db->count);
    printf("  Financial: %4d (%5.1f%%)\n", financial, 100.0*financial/db->count);
    printf("  PHI:       %4d (%5.1f%%)\n", phi, 100.0*phi/db->count);
    printf("  Neutral:   %4d (%5.1f%%)\n", neutral, 100.0*neutral/db->count);
    printf("\n");
    
    // Memory usage
    size_t concept_size = sizeof(Concept);
    size_t total_size = sizeof(ConceptDatabase);
    size_t embedding_size = db->count * db->embedding_dim * sizeof(float);
    
    printf("Memory usage:\n");
    printf("  Per concept:      %zu bytes\n", concept_size);
    printf("  All concepts:     %.2f KB\n", (db->count * concept_size) / 1024.0);
    printf("  Embeddings only:  %.2f KB\n", embedding_size / 1024.0);
    printf("  Total database:   %.2f MB\n", total_size / (1024.0 * 1024.0));
}
