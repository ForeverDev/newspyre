#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdio.h>
#include <stdint.h>
#include "lexer.h"

typedef struct Assembler Assembler;
typedef struct AssemblerFile AssemblerFile;
typedef struct AssemblerLabel AssemblerLabel;
typedef struct AssemblerInstruction AssemblerInstruction;
typedef enum AssemblerOperand AssemblerOperand;

enum AssemblerOperand {
	NO_OPERAND = 0,
	INT64,
	INT32,
	FLOAT64
};

struct Assembler {
	Token*				tokens;
	AssemblerLabel*		labels;
};

struct AssemblerFile {
	FILE*				handle;
	unsigned long long	length;
	char*				contents;
};

struct AssemblerLabel {
	char*				identifier;
	uint32_t			index;
	AssemblerLabel*		next;
};

struct AssemblerInstruction {
	char*				name;
	uint8_t				opcode;
	AssemblerOperand	operands[4];
};

extern const AssemblerInstruction instructions[0xFF];

void Assembler_generateBytecodeFile(const char*);
static void	Assembler_die(Assembler*, const char*, ...);
static void Assembler_appendLabel(Assembler*, const char*, uint32_t);
static const AssemblerInstruction* Assembler_validateInstruction(Assembler*, const char*);
static int strcmp_lower(const char*, const char*);

#endif
