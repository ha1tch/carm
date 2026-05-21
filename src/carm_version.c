/*
 * carm_version.c
 *
 * CARM — Controlled Attention Routing and Masking
 * Version utility functions.
 */
/*
 * carm_version.c
 *
 * CARM -- Controlled Attention Routing and Masking
 * Version utility functions
 *
 * Copyright (c) 2026 haitch
 * Licensed under the Apache License, Version 2.0
 * https://www.apache.org/licenses/LICENSE-2.0
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "carm_version.h"

static void parse_version(int *major, int *minor, int *patch) {
    *major = 0; *minor = 0; *patch = 0;
    /* CARM_VERSION is "X.Y.Z" or "X.Y.Z-suffix" */
    char buf[32];
    strncpy(buf, CARM_VERSION, 31);
    buf[31] = '\0';
    char *p = buf;
    *major = (int)strtol(p, &p, 10);
    if (*p == '.') { p++; *minor = (int)strtol(p, &p, 10); }
    if (*p == '.') { p++; *patch = (int)strtol(p, &p, 10); }
}

int carm_version_major(void) {
    int maj, min, pat;
    parse_version(&maj, &min, &pat);
    return maj;
}

int carm_version_minor(void) {
    int maj, min, pat;
    parse_version(&maj, &min, &pat);
    return min;
}

int carm_version_patch(void) {
    int maj, min, pat;
    parse_version(&maj, &min, &pat);
    return pat;
}

void carm_version_print(void) {
    printf("CARM v%s\n", CARM_VERSION);
}
