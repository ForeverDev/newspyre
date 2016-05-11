#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "assembler.h"

void
Assembler_generateBytecodeFile(const char* in_file_name) {
	AssemblerInput input;
	input.handle = fopen(in_file_name, "r");
	if (!input.handle) {
		Assembler_die("Couldn't open source file '%s'", in_file_name);	
	}
	fseek(input.handle, 0, SEEK_END);
	input.length = ftell(input.handle);
	fseek(input.handle, 0, SEEK_SET);
	input.contents = (char *)malloc(input.length + 1);
	fread(input.contents, 1, input.length, input.handle);

	Assembler A;
	A.tokens = Lexer_convertToTokens(input.contents);
	
	free(A.tokens);
	free(input.handle);
	free(input.contents);
}

static void
Assembler_die(const char* format, ...) {
	va_list list;
	printf("\n*** Spyre assembler error ***\n");
	va_start(list, format);
	vprintf(format, list);
	va_end(list);
	printf("\n\n");
	exit(1);
}
