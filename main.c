#include "spyre.h"

int main(int argc, char** argv) {

	static const uint8_t bytecode[] = {
		0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 
	};

	SpyState* S = Spy_newState(SPY_NOFLAG);
	Spy_execute(S, bytecode);

	return 0;

}
