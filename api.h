#ifndef API_H
#define API_H

#include "spyre.h"

void SpyL_initialize(SpyState*);

/* stdio */
static uint32_t SpyL_println(SpyState*);

/* memory management */
static uint32_t SpyL_malloc(SpyState*);

/* math */
static uint32_t SpyL_max(SpyState*);
static uint32_t SpyL_min(SpyState*);
static uint32_t SpyL_map(SpyState*);

#endif
