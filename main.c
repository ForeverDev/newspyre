#include <stdio.h>
#include "spyre.h"

int main(int argc, char** argv) {
	
	// printf("%d\n", 5);
	static const uint8_t bytecode[] = {
		0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x18,
			0x02, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00,
		0x00
	};

	static const uint8_t static_memory[] = "println\0%d\n";
	static const size_t  static_memory_size = sizeof(static_memory) / sizeof(uint8_t);

	SpyState* S = Spy_newState(SPY_NOFLAG);
	Spy_execute(S, bytecode, static_memory, static_memory_size);
	Spy_dump(S);

	return 0;

}
