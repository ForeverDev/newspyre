#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdio.h>
#include "lexer.h"

typedef struct Assembler Assembler;
typedef struct AssemblerInput AssemblerInput;

struct AssemblerInput {
	FILE*				handle;
	unsigned long long	length;
	char*				contents;
};

struct Assembler {
	Token* tokens;
};

void			Assembler_generateBytecodeFile(const char*);
static void		Assembler_die(const char*, ...);

#endif
