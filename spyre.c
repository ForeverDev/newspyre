#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "spyre.h"
#include "api.h"

SpyState*
Spy_newState(uint32_t option_flags) {
	SpyState* S = (SpyState *)malloc(sizeof(SpyState));
	S->memory = (uint8_t *)calloc(1, SIZE_MEMORY);
	S->ip = NULL; /* to be assigned when code is executed */
	S->sp = &S->memory[START_STACK - 1]; /* stack grows upwards */
	S->bp = &S->memory[START_STACK - 1];
	S->option_flags = option_flags;
	S->runtime_flags = 0;
	S->c_functions = NULL;
	S->memory_chunks = NULL;
	SpyL_initializeStandardLibrary(S);
	Spy_log(S, "Spyre state created, %d bytes of memory (%d %s)\n", 
		SIZE_MEMORY, 
		(SIZE_MEMORY / 0x400) > 0x400 ?: (SIZE_MEMORY / 0x100000),
		(SIZE_MEMORY / 0x400) > 0x400 ? "KiB" : "MiB"
	);
	return S;
}

void 
Spy_log(SpyState* S, const char* format, ...) {
	if (!(S->option_flags | SPY_DEBUG)) return;
	va_list list;
	va_start(list, format);
	vprintf(format, list);
	va_end(list);
}

void
Spy_crash(SpyState* S, const char* format, ...) {
	printf("SPYRE RUNTIME ERROR: ");
	va_list list;
	va_start(list, format);
	vprintf(format, list);
	va_end(list);
	exit(1);
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

inline void
Spy_pushFloat(SpyState* S, double value) {
	S->sp += 8;
	*(double *)S->sp = value;
}

inline double
Spy_readFloat(SpyState* S) {
	S->ip += 8;
	return *(double *)(S->ip - 8);
}

inline double
Spy_popFloat(SpyState* S) {
	double result = *(double *)S->sp;
	S->sp -= 8;
	return result;
}

void
Spy_dump(SpyState* S) {
	for (const uint8_t* i = &S->memory[SIZE_ROM]; i != S->sp + 1; i++) {
		printf("0x%08lx: %02x\n", i - S->memory, *i);	
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
Spy_execute(SpyState* S, const uint8_t* bytecode, const uint8_t* static_memory, size_t static_memory_size) {
	
	/* load static memory into ROM section */
	for (size_t i = 0; i < static_memory_size; i++) {
		S->memory[i] = static_memory[i];
	}

	/* prepare instruction pointer, point it to code */	
	S->ip = &bytecode[0];
	
	/* general purpose vars for interpretation */
	int64_t a;

	/* pointers to labels, (direct threading, significantly faster than switch/case) */
	static const void* opcodes[] = {
		&&noop, &&ipush, &&iadd, &&isub,
		&&imul, &&idiv, &&mod, &&shl, 
		&&shr, &&and, &&or, &&xor, &&not,
		&&neg, &&igt, &&ige, &&ilt,
		&&ile, &&icmp, &&jnz, &&jz,
		&&jmp, &&call, &&iret, &&ccall,
		&&fpush, &&fadd, &&fsub, &&fmul,
		&&fdiv, &&fgt, &&fge, &&flt, 
		&&fle, &&fcmp
	};

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
		while (cf && strcmp(cf->identifier, (const char *)&S->memory[name_index])) cf = cf->next;
		if (cf) {
			/* note -1 represents vararg */
			if (cf->nargs != nargs && cf->nargs >= 0) {
				Spy_crash(S, "Attempt to call C function '%s' with the incorret number of arguments (wanted %d, got %d)\n", 
					&S->memory[name_index],
					cf->nargs,
					nargs
				);
			} 
			cf->function(S);
		} else {
			Spy_crash(S, "Attempt to call undefined C function '%s'\n", &S->memory[name_index]);
		}
	}
	goto dispatch;
		
	fpush:
	Spy_pushFloat(S, Spy_readFloat(S));
	goto dispatch;

	fadd:
	Spy_pushFloat(S, Spy_popFloat(S) + Spy_popFloat(S));
	goto dispatch;

	fsub:
	a = Spy_popFloat(S);
	Spy_pushFloat(S, Spy_popFloat(S) - a);
	goto dispatch;

	fmul:
	Spy_pushFloat(S, Spy_popFloat(S) * Spy_popFloat(S));
	goto dispatch;

	fdiv:
	a = Spy_popFloat(S);
	Spy_pushFloat(S, Spy_popFloat(S) / a);
	goto dispatch;

	fgt:
	a = Spy_popFloat(S);
	Spy_pushFloat(S, Spy_popFloat(S) > a);
	goto dispatch;

	fge:
	a = Spy_popFloat(S);
	Spy_pushFloat(S, Spy_popFloat(S) >= a);
	goto dispatch;

	flt:
	a = Spy_popFloat(S);
	Spy_pushFloat(S, Spy_popFloat(S) < a);
	goto dispatch;

	fle:
	a = Spy_popFloat(S);
	Spy_pushFloat(S, Spy_popFloat(S) <= a);
	goto dispatch;

	fcmp:
	Spy_pushInt(S, Spy_popFloat(S) == Spy_popFloat(S));
	goto dispatch;

	done:
	return;

}
