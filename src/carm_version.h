/*
 * carm_version.h
 *
 * CARM — Controlled Attention Routing and Masking
 * Version header — generated from VERSION file at build time.
 *
 * Do not edit manually. Update VERSION and run make.
 */
/*
 * carm_version.h
 *
 * CARM -- Controlled Attention Routing and Masking
 * Version header: exposes CARM_VERSION injected at compile time
 *
 * Copyright (c) 2026 haitch
 * Licensed under the Apache License, Version 2.0
 * https://www.apache.org/licenses/LICENSE-2.0
 */


#ifndef CARM_VERSION_H
#define CARM_VERSION_H

/*
 * CARM_VERSION is injected by the Makefile via -DCARM_VERSION=\"x.y.z\"
 * This fallback is used only when building outside the Makefile.
 */
#ifndef CARM_VERSION
#define CARM_VERSION "0.0.0-dev"
#endif

/* Parsed components — derived from CARM_VERSION at runtime via carm_version_parse(). */
int carm_version_major(void);
int carm_version_minor(void);
int carm_version_patch(void);

/* Print "CARM vX.Y.Z" to stdout. */
void carm_version_print(void);

#endif /* CARM_VERSION_H */
