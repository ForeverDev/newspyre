#ifndef API_H
#define API_H

#include "spyre.h"

void SpyL_initializeStandardLibrary(SpyState*);

/* stdio */
static uint32_t SpyL_println(SpyState*);

/* file system */
static uint32_t SpyL_fopen(SpyState*);
static uint32_t SpyL_fclose(SpyState*);
static uint32_t SpyL_fputc(SpyState*);

/* memory management */
static uint32_t SpyL_malloc(SpyState*);
static uint32_t SpyL_free(SpyState*);

/* math */
static uint32_t SpyL_max(SpyState*);
static uint32_t SpyL_min(SpyState*);
static uint32_t SpyL_map(SpyState*);

#endif
