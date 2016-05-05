#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "Spyre.h"

SpyState*
Spy_newState(uint32_t flags) {
	SpyState* S = (SpyState *)malloc(sizeof(SpyState));
	S->memory = (uint8_t *)malloc(SIZE_MEMORY);
	S->ip = NULL; /* to be assigned when code is executed */
	S->sp = &S->memory[0] - 1; /* stack grows upwards */
	S->bp = &S->memory[0] - 1;
	S->flags = flags;
	Spy_log(S, "Spyre state created, 0x%08x bytes of memory\n", SIZE_MEMORY);
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
Spy_pushIntCode(SpyState* S) {
	S->sp += 8;
	*(int64_t *)S->sp = *(int64_t *)S->ip;
	S->ip += 8;
}

void
Spy_pushIntValue(SpyState* S, int64_t value) {
	S->sp += 8;
	*(int64_t *)S->sp = value;
}

int64_t
Spy_popInt(SpyState* S) {
	int64_t result = *(int64_t *)S->sp;
	S->sp -= 8;
	return result;
}

void
Spy_dump(SpyState* S) {
	for (const uint8_t* i = S->memory; i != S->sp + 1; i++) {
		printf("0x%08llx: %02x\n", (uint64_t)(i - S->memory), *i);	
	}
}

void
Spy_execute(SpyState* S, const uint8_t* bytecode) {
	uint8_t opcode;
	int64_t a; /* general purpose vars for interpretation */
	static const void* dispatch[] = {
		&&noop, &&ipush, &&iadd, &&isub,
		&&imul, &&idiv, &&mod, &&shl, 
		&&shr, &&and, &&or, &&xor
	};
	S->ip = &bytecode[0];
	/* main interpreter loop */
	next:
	goto *dispatch[*S->ip++];

	noop:
	goto done;
	
	ipush:
	Spy_pushIntCode(S);
	goto next;

	iadd:
	Spy_pushIntValue(S, Spy_popInt(S) + Spy_popInt(S));
	goto next;

	isub:
	a = Spy_popInt(S);
	Spy_pushIntValue(S, Spy_popInt(S) - a);
	goto next;

	imul:
	Spy_pushIntValue(S, Spy_popInt(S) * Spy_popInt(S));
	goto next;

	idiv:
	a = Spy_popInt(S);
	Spy_pushIntValue(S, Spy_popInt(S) / a);
	goto next;

	mod:
	a = Spy_popInt(S);
	Spy_pushIntValue(S, Spy_popInt(S) % a);
	goto next;

	shl:
	a = Spy_popInt(S);
	Spy_pushIntValue(S, Spy_popInt(S) << a);
	goto next;
	
	shr:
	a = Spy_popInt(S);
	Spy_pushIntValue(S, Spy_popInt(S) >> a);
	goto next;

	and:
	a = Spy_popInt(S);
	Spy_pushIntValue(S, Spy_popInt(S) & a);
	goto next;

	or:
	a = Spy_popInt(S);
	Spy_pushIntValue(S, Spy_popInt(S) | a);
	goto next;

	xor:
	a = Spy_popInt(S);
	Spy_pushIntValue(S, Spy_popInt(S) ^ a);
	goto next;

	not:
	Spy_pushIntValue(S, ~Spy_popInt(S));
	goto next;

	done:
	return;
}
