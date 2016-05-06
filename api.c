#include <stdio.h>
#include "api.h"

void SpyL_initialize(SpyState* S) {
	Spy_pushC(S, "printf", SpyL_printf, -1);
	Spy_pushC(S, "multiply", SpyL_multiply, 2);
}

static uint32_t
SpyL_printf(SpyState* S) {
	printf("%s\n", &S->memory[Spy_popInt(S)]);
	return 0;
}

static uint32_t
SpyL_multiply(SpyState* S) {
	Spy_pushInt(S, Spy_popInt(S) * Spy_popInt(S));
	return 1;
}
