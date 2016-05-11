#include "spyre.h"
#include "assembler.h"

int main(int argc, char** argv) {
	
	if (argc <= 1) return printf("expected file name\n");

	Assembler_generateBytecodeFile(argv[1]);

	return 0;

}
