#ifndef SPYRE_H
#define SPYRE_H

#include <stddef.h>
#include <stdint.h>

/* flags */
#define SPY_NOFLAG	0x00
#define SPY_DEBUG	0x01

/* constants */
#define SIZE_MEMORY 0x10000
#define SIZE_STACK	0x01000

typedef struct SpyState SpyState;

struct SpyState {
	uint8_t*		memory; /* stack from [0, SIZE_STACK), heap from [SIZE_STACK, SIZE_MEMORY) */
	const uint8_t*	ip;
	uint8_t*		sp;
	uint8_t*		bp;
	uint32_t		flags;
};

SpyState*	Spy_newState(uint32_t);
void		Spy_log(SpyState*, const char*, ...);
void		Spy_pushInt(SpyState*);
void		Spy_execute(SpyState*, const uint8_t*);

#endif
