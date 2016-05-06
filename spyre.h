#ifndef SPYRE_H
#define SPYRE_H

#include <stddef.h>
#include <stdint.h>

/* option flags */
#define SPY_NOFLAG	0x00
#define SPY_DEBUG	0x01

/* runtime flags */
#define SPY_CMPRESULT 0x01

/* constants */
#define SIZE_MEMORY 0x100000
#define SIZE_STACK	0x010000
#define SIZE_ROM	0x010000

typedef struct SpyState SpyState;
typedef struct SpyCFunction SpyCFunction;
typedef struct SpyMemoryChunk SpyMemoryChunk;


struct SpyCFunction {
	const char*		identifier;
	uint32_t		(*function)(SpyState*);
	int				nargs;
	SpyCFunction*	next;
};

struct SpyMemoryChunk {
	size_t			size;
	uint8_t*		absolute_address;
	uint64_t		vm_address;
	SpyMemoryChunk*	next;
};

struct SpyState {
	uint8_t*		memory;
	const uint8_t*	ip;
	uint8_t*		sp;
	uint8_t*		bp;
	uint32_t		option_flags;
	uint32_t		runtime_flags;
	SpyCFunction*	c_functions;
	SpyMemoryChunk*	memory_chunks;
};

SpyState*	Spy_newState(uint32_t);
void		Spy_log(SpyState*, const char*, ...);
void		Spy_crash(SpyState*, const char*, ...);
void		Spy_dump(SpyState*);
void		Spy_pushInt(SpyState*, int64_t);
uint32_t	Spy_readInt32(SpyState*);
uint64_t	Spy_readInt64(SpyState*);
int64_t 	Spy_popInt(SpyState*);
void		Spy_pushFloat(SpyState*, double);
double		Spy_readFloat(SpyState*);
double		Spy_popFloat(SpyState*);
void		Spy_pushC(SpyState*, const char*, uint32_t (*)(SpyState*), int);
void		Spy_execute(SpyState*, const uint8_t*, const uint8_t*, size_t);

#endif
