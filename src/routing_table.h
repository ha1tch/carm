/*
 * routing_table.h
 *
 * CARM -- Controlled Attention Routing and Masking
 * O(1) routing array: API, inline lookup, and TOSID macro definitions
 *
 * Copyright (c) 2026 haitch
 * Licensed under the Apache License, Version 2.0
 * https://www.apache.org/licenses/LICENSE-2.0
 */
#ifndef ROUTING_TABLE_H
#define ROUTING_TABLE_H

#include <stdint.h>
#include <string.h>

// Routing actions
#define ROUTE_DENY  0
#define ROUTE_ALLOW 1

// TOSID format: 0xDDCCVVVV
// DD = Domain (8 bits)
// CC = Category (8 bits)
// VVVV = Variant (16 bits)
//
// Routing operates on prefix (top 16 bits): DD + CC

// Size: 2^16 = 65,536 entries (64 KB)
#define ROUTING_TABLE_SIZE 65536

// Routing table structure
typedef struct {
    uint8_t actions[ROUTING_TABLE_SIZE];  // 64 KB
    char context_name[64];
    int rule_count;
} FastRoutingTable;

// Routing rule (for setup/configuration)
typedef struct {
    uint32_t prefix;   // Prefix to match (e.g., 0x10C50000)
    uint32_t mask;     // Mask for matching (e.g., 0xFFFF0000)
    int action;        // ROUTE_ALLOW or ROUTE_DENY
} FastRoutingRule;

// Initialize routing table (default: DENY all)
void routing_table_init(FastRoutingTable* table, const char* context_name);

// Add a routing rule (applies prefix/mask to table)
void routing_table_add_rule(FastRoutingTable* table, const FastRoutingRule* rule);

// Add multiple rules at once
void routing_table_add_rules(FastRoutingTable* table, 
                             const FastRoutingRule* rules, 
                             int count);

// Print routing table statistics
void routing_table_print_stats(const FastRoutingTable* table);

// O(1) routing check - CRITICAL PERFORMANCE PATH
// This function is inlined for maximum performance
static inline int routing_table_check(uint32_t tosid, const FastRoutingTable* table) {
    // Extract top 16 bits (domain + category)
    uint16_t index = (tosid >> 16) & 0xFFFF;
    
    // Single array lookup - O(1)
    return table->actions[index] == ROUTE_ALLOW;
}

// Alternative: Check with explicit return of action
static inline int routing_table_get_action(uint32_t tosid, const FastRoutingTable* table) {
    uint16_t index = (tosid >> 16) & 0xFFFF;
    return table->actions[index];
}

// TOSID manipulation helpers
#define TOSID_GET_DOMAIN(tosid)   (((tosid) >> 24) & 0xFF)
#define TOSID_GET_CATEGORY(tosid) (((tosid) >> 16) & 0xFF)
#define TOSID_GET_VARIANT(tosid)  ((tosid) & 0xFFFF)
#define TOSID_GET_PREFIX(tosid)   (((tosid) >> 16) & 0xFFFF)

// Create TOSID from components
#define TOSID_CREATE(domain, category, variant) \
    ((((uint32_t)(domain) & 0xFF) << 24) | \
     (((uint32_t)(category) & 0xFF) << 16) | \
     ((uint32_t)(variant) & 0xFFFF))

// Create prefix from domain + category
#define PREFIX_CREATE(domain, category) \
    ((((uint32_t)(domain) & 0xFF) << 8) | ((uint32_t)(category) & 0xFF))

// Standard masks
#define MASK_DOMAIN_ONLY     0xFF000000  // Match domain only (top 8 bits)
#define MASK_DOMAIN_CATEGORY 0xFFFF0000  // Match domain + category (top 16 bits)
#define MASK_ALL             0xFFFFFFFF  // Match exact TOSID

#endif // ROUTING_TABLE_H
