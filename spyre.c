#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "Spyre.h"

SpyState*
Spy_newState(uint32_t flags) {
	SpyState* S = (SpyState *)malloc(sizeof(SpyState));
	S->memory = (uint8_t *)calloc(1, SIZE_MEMORY);
	S->ip = NULL; /* to be assigned when code is executed */
	S->sp = &S->memory[0] - 1; /* stack grows upwards */
	S->bp = &S->memory[0] - 1;
	S->flags = flags;
	S->c_functions = NULL;
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

inline void
Spy_pushInt(SpyState* S, int64_t value) {
	S->sp += 8;
	*(int64_t *)S->sp = value;
}

inline uint32_t
Spy_readInt32(SpyState* S) {
	S->ip += 4;
	return *(uint32_t *)(S->ip - 4);
}

inline uint64_t
Spy_readInt64(SpyState* S) {
	S->ip += 8;
	return *(uint64_t *)(S->ip - 8);
}

inline int64_t
Spy_popInt(SpyState* S) {
	int64_t result = *(int64_t *)S->sp;
	S->sp -= 8;
	return result;
}

void
Spy_dump(SpyState* S) {
	for (const uint8_t* i = S->memory; i != S->sp + 1; i++) {
		printf("0x%08llx: %02x\n", (uintptr_t)(i - S->memory), *i);	
	}
}

void
Spy_pushC(SpyState* S, const char* identifier, uint32_t (*function)(SpyState*), int nargs) {
	SpyCFunction* container = (SpyCFunction *)malloc(sizeof(SpyCFunction));
	container->identifier = identifier;
	container->function = function;
	container->next = NULL;
	container->nargs = nargs;
	if (!S->c_functions) {
		S->c_functions = container;
	} else {
		SpyCFunction* at = S->c_functions;
		while (at->next) at = at->next;
		at->next = container;
	}
}

void
Spy_execute(SpyState* S, const uint8_t* bytecode, const uint8_t* static_memory) {
	uint8_t opcode;
	int64_t a; /* general purpose vars for interpretation */
	static const void* opcodes[] = {
		&&noop, &&ipush, &&iadd, &&isub,
		&&imul, &&idiv, &&mod, &&shl, 
		&&shr, &&and, &&or, &&xor, &&not,
		&&neg, &&igt, &&ige, &&ilt,
		&&ile, &&icmp, &&jnz, &&jz,
		&&jmp, &&call, &&iret, &&ccall
	};
	S->ip = &bytecode[0];
	/* main interpreter loop */
	dispatch:
	goto *opcodes[*S->ip++];

	noop:
	goto done;
	
	ipush:
	Spy_pushInt(S, Spy_readInt64(S));
	goto dispatch;

	iadd:
	Spy_pushInt(S, Spy_popInt(S) + Spy_popInt(S));
	goto dispatch;

	isub:
	a = Spy_popInt(S);
	Spy_pushInt(S, Spy_popInt(S) - a);
	goto dispatch;

	imul:
	Spy_pushInt(S, Spy_popInt(S) * Spy_popInt(S));
	goto dispatch;

	idiv:
	a = Spy_popInt(S);
	Spy_pushInt(S, Spy_popInt(S) / a);
	goto dispatch;

	mod:
	a = Spy_popInt(S);
	Spy_pushInt(S, Spy_popInt(S) % a);
	goto dispatch;

	shl:
	a = Spy_popInt(S);
	Spy_pushInt(S, Spy_popInt(S) << a);
	goto dispatch;
	
	shr:
	a = Spy_popInt(S);
	Spy_pushInt(S, Spy_popInt(S) >> a);
	goto dispatch;

	and:
	a = Spy_popInt(S);
	Spy_pushInt(S, Spy_popInt(S) & a);
	goto dispatch;

	or:
	a = Spy_popInt(S);
	Spy_pushInt(S, Spy_popInt(S) | a);
	goto dispatch;

	xor:
	a = Spy_popInt(S);
	Spy_pushInt(S, Spy_popInt(S) ^ a);
	goto dispatch;

	not:
	Spy_pushInt(S, ~Spy_popInt(S));
	goto dispatch;

	neg:
	Spy_pushInt(S, -Spy_popInt(S));
	goto dispatch;

	igt:
	a = Spy_popInt(S);
	Spy_pushInt(S, Spy_popInt(S) > a);
	goto dispatch;

	ige:
	a = Spy_popInt(S);
	Spy_pushInt(S, Spy_popInt(S) >= a);
	goto dispatch;

	ilt:
	a = Spy_popInt(S);
	Spy_pushInt(S, Spy_popInt(S) < a);
	goto dispatch;

	ile:
	a = Spy_popInt(S);
	Spy_pushInt(S, Spy_popInt(S) <= a);
	goto dispatch;

	icmp:
	Spy_pushInt(S, Spy_popInt(S) == Spy_popInt(S));
	goto dispatch;

	jnz:
	if (Spy_popInt(S)) {
		S->ip = (uint8_t *)(bytecode + Spy_readInt32(S));
	}
	goto dispatch;

	jz:
	if (!Spy_popInt(S)) {
		S->ip = (uint8_t *)(bytecode + Spy_readInt32(S));
	}
	goto dispatch;

	jmp:
	S->ip = (uint8_t *)(bytecode + Spy_readInt32(S));
	goto dispatch;

	call:
	Spy_pushInt(S, Spy_readInt32(S)); /* push number of arguments */
	Spy_pushInt(S, (uintptr_t)S->bp); /* push base pointer */
	Spy_pushInt(S, (uintptr_t)S->ip); /* push return address */
	S->bp = S->sp;
	S->ip = (uint8_t *)(bytecode + Spy_readInt32(S));
	goto dispatch;

	iret:
	a = Spy_popInt(S); /* return value */
	S->sp = S->bp;
	S->ip = (uint8_t *)(intptr_t)Spy_popInt(S);	
	S->bp = (uint8_t *)(intptr_t)Spy_popInt(S);
	S->sp -= Spy_popInt(S);
	Spy_pushInt(S, a);
	goto dispatch;	

	ccall:
	{
		uint32_t nargs = Spy_readInt32(S);
		uint32_t name_index = Spy_readInt32(S);
		SpyCFunction* cf = S->c_functions;
		while (cf && strcmp(cf->identifier, &static_memory[name_index])) {cf = cf->next; printf("meme\n"); }
		if (cf) {
			/* TODO throw runtime error, incorrect number of arguments */
			/* note -1 represents vararg */
			if (cf->nargs != nargs && cf->nargs >= 0) {
				printf("Attempt to call C function '%s' with the incorrect number of arguments\n");
				exit(1);
			} 
			cf->function(S);
		} else {
			/* TODO throw runtime error, attempt to call undefined C function */
			printf("Attempt to call undefined C function '%s'\n", &static_memory[name_index]);
			exit(1);
		}
	}
	goto dispatch;

	done:
	return;
}
