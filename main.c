#include <string.h>
#include "spyre.h"
#include "assembler.h"

int main(int argc, char** argv) {
	
	if (argc <= 1) return printf("expected file name\n");

	char* args[] = {"examples/test.spy"};

	if (!strncmp(argv[1], "c", 1)) {
		Assembler_generateBytecodeFile(argv[2]);
	} else if (!strncmp(argv[1], "r", 1)) {
		//Spy_execute(argv[2], SPY_NOFLAG | SPY_STEP | SPY_DEBUG, 1, args);
		Spy_execute(argv[2], SPY_NOFLAG, 1, args);
	}

	return 0;

}
