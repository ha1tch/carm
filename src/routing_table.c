/*
 * routing_table.c
 *
 * CARM -- Controlled Attention Routing and Masking
 * Routing rule application and table management
 *
 * Copyright (c) 2026 haitch
 * Licensed under the Apache License, Version 2.0
 * https://www.apache.org/licenses/LICENSE-2.0
 */
#include "routing_table.h"
#include <stdio.h>
#include <stdlib.h>

// Initialize routing table with default DENY all
void routing_table_init(FastRoutingTable* table, const char* context_name) {
    // Set all entries to DENY by default
    memset(table->actions, ROUTE_DENY, ROUTING_TABLE_SIZE);
    
    // Copy context name
    strncpy(table->context_name, context_name, 63);
    table->context_name[63] = '\0';
    
    table->rule_count = 0;
}

// Apply a single routing rule to the table
void routing_table_add_rule(FastRoutingTable* table, const FastRoutingRule* rule) {
    // For each possible TOSID prefix (16 bits)
    for (uint32_t prefix_index = 0; prefix_index < ROUTING_TABLE_SIZE; prefix_index++) {
        // Reconstruct full TOSID from prefix (shift left 16 bits)
        uint32_t tosid = prefix_index << 16;
        
        // Check if this TOSID matches the rule's prefix/mask
        if ((tosid & rule->mask) == (rule->prefix & rule->mask)) {
            // Apply action
            table->actions[prefix_index] = rule->action;
        }
    }
    
    table->rule_count++;
}

// Add multiple rules
void routing_table_add_rules(FastRoutingTable* table, 
                             const FastRoutingRule* rules, 
                             int count) {
    for (int i = 0; i < count; i++) {
        routing_table_add_rule(table, &rules[i]);
    }
}

// Print statistics about the routing table
void routing_table_print_stats(const FastRoutingTable* table) {
    int allow_count = 0;
    int deny_count = 0;
    
    for (int i = 0; i < ROUTING_TABLE_SIZE; i++) {
        if (table->actions[i] == ROUTE_ALLOW) {
            allow_count++;
        } else {
            deny_count++;
        }
    }
    
    printf("Routing Table: %s\n", table->context_name);
    printf("  Rules applied: %d\n", table->rule_count);
    printf("  ALLOW entries: %d (%.2f%%)\n", 
           allow_count, 100.0 * allow_count / ROUTING_TABLE_SIZE);
    printf("  DENY entries:  %d (%.2f%%)\n", 
           deny_count, 100.0 * deny_count / ROUTING_TABLE_SIZE);
    printf("  Table size:    %d KB\n", (int)(sizeof(table->actions) / 1024));
}
