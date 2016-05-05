#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "spyre.h"

SpyState*
Spy_newState(uint32_t flags) {
	SpyState* S = (SpyState *)malloc(sizeof(SpyState));
	S->memory = (uint8_t *)malloc(SIZE_MEMORY);
	S->ip = NULL; /* to be assigned when code is executed */
	S->sp = &S->memory[0]; /* stack grows upwards */
	S->bp = &S->memory[0];
	S->flags = flags;
	if (S->flags & SPY_DEBUG) {
		printf("Spyre state created, 0x%08x bytes of memory\n", SIZE_MEMORY);
	}
	return S;
}

void 
Spy_log(SpyState* S, const char* format, ...) {
	if (!(S->flags | SPY_DEBUG)) return;
	va_list list;
	va_start(list, format);
	vprintf(format, list);
	va_end(list);
}

void
Spy_pushInt(SpyState* S) {
	S->sp += sizeof(uint64_t);
	*(uint64_t *)S->sp = *(uint64_t *)S->ip;
	S->ip += sizeof(uint64_t);
}

void
Spy_execute(SpyState* S, const uint8_t* bytecode) {
	uint8_t opcode;
	static const void* dispatch[] = {
		&&noop, &&push_i
	};
	S->ip = &bytecode[0];
	/* main interpreter loop */
	while ((opcode = *S->ip++)) {
		goto *dispatch[opcode];

		noop:
		break;
		
		push_i:
		Spy_pushInt(S);
		continue;

	}
}
