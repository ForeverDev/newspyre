#ifndef GENERATE_H
#define GENERATE_H

#include <stdint.h>
#include "lex.h"
#include "parse.h"

typedef struct CompileState CompileState;
typedef struct InstructionStack InstructionStack;
typedef struct LiteralValue LiteralValue;

struct InstructionStack {
	char* instructions[12]; /* 64 is way too high but better safe */
	unsigned int nins; /* number of instructions so far */
	unsigned int depth;
	InstructionStack* next;
	InstructionStack* prev;
};

struct LiteralValue {
	char* name;
	LiteralValue* next;	
};

struct CompileState {
	TreeNode* node_focus;
	TreeBlock* root_block;
	LiteralValue* defined_functions;
	LiteralValue* string_literals;
	InstructionStack* ins_stack;
	unsigned int label_count;
	unsigned int literal_count;
	unsigned int depth;
	unsigned int return_label;
	unsigned int main_label;
	FILE* output;
};	

void generate_bytecode(TreeBlock*, const char*);

#endif
