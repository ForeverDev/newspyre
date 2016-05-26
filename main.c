#include <string.h>
#include "spyre.h"
#include "assembler.h"

int main(int argc, char** argv) {
	
	if (argc <= 1) return printf("expected file name\n");

	if (!strncmp(argv[1], "c", 1)) {
		Assembler_generateBytecodeFile(argv[2]);
	} else if (!strncmp(argv[1], "r", 1)) {
		Spy_execute(argv[2], SPY_NOFLAG);
	}

	return 0;

}
