#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdio.h>
#include <stdint.h>
#include "lexer.h"

typedef struct Assembler Assembler;
typedef struct AssemblerFile AssemblerFile;
typedef struct AssemblerInstruction AssemblerInstruction;
typedef enum AssemblerOperand AssemblerOperand;

enum AssemblerOperand {
	NO_OPERAND = 0,
	INT64,
	INT32,
	FLOAT64
};

struct AssemblerFile {
	FILE*				handle;
	unsigned long long	length;
	char*				contents;
};

struct AssemblerInstruction {
	char*				name;
	uint8_t				opcode;
	AssemblerOperand	operands[4];
};

struct Assembler {
	Token* tokens;
};

extern const AssemblerInstruction instructions[0xFF];

void			Assembler_generateBytecodeFile(const char*);
static void		Assembler_die(Assembler*, const char*, ...);
static uint8_t	Assembler_validateInstruction(Assembler*, const char*);

static int		strcmp_lower(const char*, const char*);

#endif
