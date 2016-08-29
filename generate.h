#ifndef GENERATE_H
#define GENERATE_H

#include <stdint.h>
#include "lex.h"
#include "parse.h"

typedef struct CompileState CompileState;
typedef struct InstructionStack InstructionStack;
typedef struct DefinedFunction DefinedFunction;

struct InstructionStack {
	char* instructions[12]; /* 64 is way too high but better safe */
	unsigned int nins; /* number of instructions so far */
	unsigned int depth;
	InstructionStack* next;
	InstructionStack* prev;
};

struct DefinedFunction {
	char* name;
	DefinedFunction* next;	
};

struct CompileState {
	TreeNode* node_focus;
	TreeBlock* root_block;
	DefinedFunction* defined_functions;
	InstructionStack* ins_stack;
	unsigned int label_count;
	unsigned int depth;
	FILE* output;
};	

void generate_bytecode(TreeBlock*, const char*);

#endif
