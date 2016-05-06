#include <stdio.h>
#include "api.h"

void SpyL_initialize(SpyState* S) {
	Spy_pushC(S, "println", SpyL_println, -1);
	Spy_pushC(S, "min", SpyL_min, -1);
	Spy_pushC(S, "max", SpyL_max, -1);
}

static uint32_t
SpyL_println(SpyState* S) {
	const char* format = (const char *)&S->memory[Spy_popInt(S)];
	while (*format) {
		switch (*format) {
			case '%':
				switch (*++format) {
					case 's':
						printf("%s", &S->memory[Spy_popInt(S)]);
						break;
					case 'd':
						printf("%lld", Spy_popInt(S));
						break;
				}
			case '\\':
				switch (*++format) {
					case 'n': printf("\n"); break;
					case 't': printf("\t"); break;
				}
			default:
				printf("%c", *format);
		}
		format++;
	}
	return 0;
}

static uint32_t
SpyL_min(SpyState* S) {
	int64_t a, b;
	a = Spy_popInt(S);
	b = Spy_popInt(S);
	Spy_pushInt(S, a < b ? a : b);
	return 1;
}

static uint32_t
SpyL_max(SpyState* S) {
	int64_t a, b;
	a = Spy_popInt(S);
	b = Spy_popInt(S);
	Spy_pushInt(S, a > b ? a : b);
	return 1;
}
