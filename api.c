#include <stdio.h>
#include <stdlib.h>
#include "api.h"

void SpyL_initializeStandardLibrary(SpyState* S) {
	Spy_pushC(S, "println", SpyL_println, -1);

	Spy_pushC(S, "fopen", SpyL_fopen, 2);
	Spy_pushC(S, "fclose", SpyL_fclose, 1);
	Spy_pushC(S, "fputc", SpyL_fputc, 2);

	Spy_pushC(S, "malloc", SpyL_malloc, 1);
	Spy_pushC(S, "free", SpyL_free, 1);

	Spy_pushC(S, "min", SpyL_min, -1);
	Spy_pushC(S, "max", SpyL_max, -1);
}

static uint32_t
SpyL_println(SpyState* S) {
	const char* format = Spy_popString(S);
	while (*format) {
		switch (*format) {
			case '%':
				switch (*++format) {
					case 's':
						printf("%s", Spy_popString(S));
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
SpyL_fopen(SpyState* S) {
	Spy_pushPointer(S, fopen(Spy_popString(S), Spy_popString(S)));
	return 1;
}

static uint32_t
SpyL_fclose(SpyState* S) {
	fclose((FILE *)Spy_popPointer(S));
	return 0;
}

/* note called as fputc(FILE*, char) */
static uint32_t
SpyL_fputc(SpyState* S) {
	FILE* f = (FILE *)Spy_popPointer(S);
	char c = (char)Spy_popInt(S);
	fputc(c, f);
	return 0;
}

static uint32_t
SpyL_malloc(SpyState* S) {
	SpyMemoryChunk* chunk = (SpyMemoryChunk *)malloc(sizeof(SpyMemoryChunk));
	if (!chunk) Spy_crash(S, "Out of memory\n");
	uint64_t size = Spy_popInt(S);

	/* round chunk->size up to nearest SIZE_PAGE multiple */
	if (size == 0) chunk->pages = 1;
	else if (size % SIZE_PAGE > 0) {
		chunk->pages = (size + (SIZE_PAGE - (size % SIZE_PAGE))) / SIZE_PAGE;
	}	
	
	/* find an open memory slot */
	if (!S->memory_chunks) {
		S->memory_chunks = chunk;
		chunk->next = NULL;
		chunk->prev = NULL;
		chunk->absolute_address = &S->memory[START_HEAP];
		chunk->vm_address = START_HEAP;
	} else {
		SpyMemoryChunk* at = S->memory_chunks;
		uint8_t found_slot = 0;
		while (at->next) {
			uint64_t offset = at->pages * SIZE_PAGE;
			uint8_t* abs_addr = at->absolute_address + offset;
			uint64_t page_distance = (at->next->absolute_address - abs_addr) / SIZE_PAGE;
			if (page_distance >= chunk->pages) {
				chunk->absolute_address = abs_addr;
				chunk->vm_address = at->vm_address + offset;
				chunk->next = at->next;
				chunk->prev = at;
				at->next->prev = chunk;
				at->next = chunk;
				found_slot = 1;
				break;
			}
		}
		if (!found_slot) {
			chunk->absolute_address = at->absolute_address + at->pages * SIZE_PAGE;
			chunk->vm_address = at->vm_address + at->pages * SIZE_PAGE;
			chunk->next = NULL;
			chunk->prev = at;
			at->next = chunk;
		}
	}

	Spy_pushInt(S, chunk->vm_address);

	return 0;
}

static uint32_t
SpyL_free(SpyState* S) {
	static const char* errmsg = "Attempt to free an invalid pointer\n";
	uint8_t success = 0;
	uint64_t vm_address = Spy_popInt(S);
	SpyMemoryChunk* at = S->memory_chunks;
	if (!at) Spy_crash(S, errmsg);
	while (at) {
		if (at->vm_address == vm_address) {
			if (at->next) {
				at->prev->next = at->next;
				at->next->prev = at->prev;
			} else {
				at->prev->next = NULL;
			}
			free(at);
			success = 1;
		}
		at = at->next; 
	}
	if (!success) {
		Spy_crash(S, errmsg);
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
