#include <stdio.h>
#include "api.h"

void SpyL_initialize(SpyState* S) {
	Spy_pushC(S, "printf", SpyL_printf, -1);
}

static uint32_t
SpyL_printf(SpyState* S) {
	printf("%s\n", &S->memory[Spy_popInt(S)]);
	return 0;
}
