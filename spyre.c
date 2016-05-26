#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "spyre.h"
#include "assembler.h"
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
	putc('\n', stdout);
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
Spy_saveInt(SpyState* S, uint8_t* addr, int64_t value) {
	*(int64_t *)addr = value;
}

inline void
Spy_pushPointer(SpyState* S, void* ptr) {
	S->sp += 8;
	*(int64_t *)S->sp = (uintptr_t)ptr;
}

inline void*
Spy_popPointer(SpyState* S) {
	void* result = (void *)(*(uintptr_t *)S->sp);
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

inline char*
Spy_popString(SpyState* S) {
	return (char *)&S->memory[Spy_popInt(S)];
}

void
Spy_dump(SpyState* S) {
	for (const uint8_t* i = &S->memory[SIZE_ROM]; i != S->sp + 1; i++) {
		printf("0x%08lx: %02x | %01c | ", i - S->memory, *i, isprint(*i) ? *i : '.');	
		if ((&S->memory[SIZE_ROM] - i - 1) % 8 == 0) {
			printf("%d\n", *(int64_t *)i);
			for (int j = 0; j < 24; j++) {
				fputc('-', stdout);
			}
		}
		fputc('\n', stdout);
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
Spy_execute(const char* filename, uint32_t option_flags) {

	SpyState S;

	S.memory = (uint8_t *)calloc(1, SIZE_MEMORY);
	S.ip = NULL; /* to be assigned when code is executed */
	S.sp = &S.memory[START_STACK - 1]; /* stack grows upwards */
	S.bp = &S.memory[START_STACK - 1];
	S.option_flags = option_flags;
	S.runtime_flags = 0;
	S.c_functions = NULL;
	S.memory_chunks = NULL;
	SpyL_initializeStandardLibrary(&S);
	Spy_log(&S, "Spyre state created, %d bytes of memory (%d %s)\n", 
		SIZE_MEMORY, 
		(SIZE_MEMORY / 0x400) > 0x400 ?: (SIZE_MEMORY / 0x100000),
		(SIZE_MEMORY / 0x400) > 0x400 ? "KiB" : "MiB"
	);

	FILE* f;
	unsigned long long flen;
	uint32_t code_start;
	f = fopen(filename, "rb");
	if (!f) Spy_crash(&S, "Couldn't open input file '%s'", filename);
	fseek(f, 0, SEEK_END);
	flen = ftell(f);
	fseek(f, 0, SEEK_SET);
	S.bytecode = (uint8_t *)malloc(flen + 1);
	fread(S.bytecode, 1, flen, f);
	S.bytecode[flen] = 0;
	fclose(f);

	for (int i = 12; i < *(uint32_t *)&S.bytecode[8]; i++) {
		S.memory[i - 12] = S.bytecode[i];
	}

	/* prepare instruction pointer, point it to code */	
	S.bytecode = &S.bytecode[*(uint32_t *)&S.bytecode[8]];
	S.ip = S.bytecode;
	
	/* general purpose vars for interpretation */
	int64_t a;
	double b;

	/* IP saver */
	uint8_t ipsave = 0;

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
		&&fle, &&fcmp, &&fret, &&ilload,
		&&ilsave, &&iarg, &&iload, &&isave
	};

	/* main interpreter loop */
	dispatch:
	if (option_flags & SPY_DEBUG) {
		for (int i = 0; i < 100; i++) {
			fputc('\n', stdout);
		}
		printf("executed %s\n", instructions[ipsave].name);
		Spy_dump(&S);
		getchar();
	}
	ipsave = *S.ip;
	goto *opcodes[*S.ip++];

	noop:
	goto done;
	
	ipush:
	Spy_pushInt(&S, Spy_readInt64(&S));
	goto dispatch;

	iadd:
	Spy_pushInt(&S, Spy_popInt(&S) + Spy_popInt(&S));
	goto dispatch;

	isub:
	a = Spy_popInt(&S);
	Spy_pushInt(&S, Spy_popInt(&S) - a);
	goto dispatch;

	imul:
	Spy_pushInt(&S, Spy_popInt(&S) * Spy_popInt(&S));
	goto dispatch;

	idiv:
	a = Spy_popInt(&S);
	Spy_pushInt(&S, Spy_popInt(&S) / a);
	goto dispatch;

	mod:
	a = Spy_popInt(&S);
	Spy_pushInt(&S, Spy_popInt(&S) % a);
	goto dispatch;

	shl:
	a = Spy_popInt(&S);
	Spy_pushInt(&S, Spy_popInt(&S) << a);
	goto dispatch;
	
	shr:
	a = Spy_popInt(&S);
	Spy_pushInt(&S, Spy_popInt(&S) >> a);
	goto dispatch;

	and:
	a = Spy_popInt(&S);
	Spy_pushInt(&S, Spy_popInt(&S) & a);
	goto dispatch;

	or:
	a = Spy_popInt(&S);
	Spy_pushInt(&S, Spy_popInt(&S) | a);
	goto dispatch;

	xor:
	a = Spy_popInt(&S);
	Spy_pushInt(&S, Spy_popInt(&S) ^ a);
	goto dispatch;

	not:
	Spy_pushInt(&S, ~Spy_popInt(&S));
	goto dispatch;

	neg:
	Spy_pushInt(&S, -Spy_popInt(&S));
	goto dispatch;

	igt:
	a = Spy_popInt(&S);
	Spy_pushInt(&S, Spy_popInt(&S) > a);
	goto dispatch;

	ige:
	a = Spy_popInt(&S);
	Spy_pushInt(&S, Spy_popInt(&S) >= a);
	goto dispatch;

	ilt:
	a = Spy_popInt(&S);
	Spy_pushInt(&S, Spy_popInt(&S) < a);
	goto dispatch;

	ile:
	a = Spy_popInt(&S);
	Spy_pushInt(&S, Spy_popInt(&S) <= a);
	goto dispatch;

	icmp:
	Spy_pushInt(&S, Spy_popInt(&S) == Spy_popInt(&S));
	goto dispatch;

	jnz:
	a = Spy_readInt32(&S);
	if (Spy_popInt(&S)) {
		S.ip = (uint8_t *)&S.bytecode[a];
	}
	goto dispatch;

	jz:
	a = Spy_readInt32(&S);
	if (!Spy_popInt(&S)) {
		S.ip = (uint8_t *)&S.bytecode[a];
	}
	goto dispatch;

	jmp:
	S.ip = (uint8_t *)&S.bytecode[Spy_readInt32(&S)];
	goto dispatch;

	call:
	a = Spy_readInt32(&S);
	Spy_pushInt(&S, Spy_readInt32(&S)); /* push number of arguments */
	Spy_pushInt(&S, (uintptr_t)S.bp); /* push base pointer */
	Spy_pushInt(&S, (uintptr_t)S.ip); /* push return address */
	S.bp = S.sp;
	S.ip = (uint8_t *)&S.bytecode[a];
	goto dispatch;

	iret:
	a = Spy_popInt(&S); /* return value */
	S.sp = S.bp;
	S.ip = (uint8_t *)(intptr_t)Spy_popInt(&S);	
	S.bp = (uint8_t *)(intptr_t)Spy_popInt(&S);
	S.sp -= Spy_popInt(&S) * 8;
	Spy_pushInt(&S, a);
	goto dispatch;	

	ccall:
	{
		uint32_t name_index = Spy_readInt32(&S);
		uint32_t nargs = Spy_readInt32(&S);
		SpyCFunction* cf = S.c_functions;
		while (cf && strcmp(cf->identifier, (const char *)&S.memory[name_index])) cf = cf->next;
		if (cf) {
			/* note -1 represents vararg */
			if (cf->nargs != nargs && cf->nargs >= 0) {
				Spy_crash(&S, "Attempt to call C function '%s' with the incorret number of arguments (wanted %d, got %d)\n", 
					&S.memory[name_index],
					cf->nargs,
					nargs
				);
			} 
			cf->function(&S);
		} else {
			Spy_crash(&S, "Attempt to call undefined C function '%s'\n", &S.memory[name_index]);
		}
	}
	goto dispatch;
		
	fpush:
	Spy_pushFloat(&S, Spy_readFloat(&S));
	goto dispatch;

	fadd:
	Spy_pushFloat(&S, Spy_popFloat(&S) + Spy_popFloat(&S));
	goto dispatch;

	fsub:
	b = Spy_popFloat(&S);
	Spy_pushFloat(&S, Spy_popFloat(&S) - b);
	goto dispatch;

	fmul:
	Spy_pushFloat(&S, Spy_popFloat(&S) * Spy_popFloat(&S));
	goto dispatch;

	fdiv:
	b = Spy_popFloat(&S);
	Spy_pushFloat(&S, Spy_popFloat(&S) / b);
	goto dispatch;

	fgt:
	b = Spy_popFloat(&S);
	Spy_pushFloat(&S, Spy_popFloat(&S) > b);
	goto dispatch;

	fge:
	b = Spy_popFloat(&S);
	Spy_pushFloat(&S, Spy_popFloat(&S) >= b);
	goto dispatch;

	flt:
	b = Spy_popFloat(&S);
	Spy_pushFloat(&S, Spy_popFloat(&S) < b);
	goto dispatch;

	fle:
	b = Spy_popFloat(&S);
	Spy_pushFloat(&S, Spy_popFloat(&S) <= b);
	goto dispatch;

	fcmp:
	Spy_pushInt(&S, Spy_popFloat(&S) == Spy_popFloat(&S));
	goto dispatch;

	fret:
	b = Spy_popFloat(&S); /* return value */
	S.sp = S.bp;
	S.ip = (uint8_t *)(uintptr_t)Spy_popInt(&S);	
	S.bp = (uint8_t *)(uintptr_t)Spy_popInt(&S);
	S.sp -= Spy_popInt(&S);
	Spy_pushFloat(&S, a);
	goto dispatch;	

	ilload:
	Spy_pushInt(&S, *(int64_t *)&S.bp[Spy_readInt32(&S) * sizeof(uint64_t)]);
	goto dispatch;

	ilsave:
	Spy_saveInt(&S, &S.bp[Spy_readInt32(&S) * sizeof(uint64_t)], Spy_popInt(&S));
	goto dispatch;

	iarg:
	Spy_pushInt(&S, *(int64_t *)&S.bp[-2*8 - Spy_readInt32(&S)*8]);
	goto dispatch;

	iload:
	Spy_pushInt(&S, *(int64_t *)&S.memory[Spy_popInt(&S)]);
	goto dispatch;

	isave:
	a = Spy_popInt(&S); /* pop value */
	Spy_saveInt(&S, &S.memory[Spy_popInt(&S)], a);
	goto dispatch;

	done:
	Spy_dump(&S);

	return;

}
