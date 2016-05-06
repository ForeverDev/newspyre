#include "spyre.h"

int main(int argc, char** argv) {

	static const uint8_t bytecode[] = {
		0x18, /* ccall */
			0x00, 0x00, 0x00, 0x00, /* 0 args */
			0x00, 0x00, 0x00, 0x00, /* printf */
		0x00
	};

	static const uint8_t* static_memory = "printf\0sin";

	SpyState* S = Spy_newState(SPY_NOFLAG);
	Spy_execute(S, bytecode, static_memory);
	Spy_dump(S);

	return 0;

}
