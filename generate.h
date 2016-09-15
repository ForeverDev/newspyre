#ifndef GENERATE_H
#define GENERATE_H

#include <stdint.h>
#include "lex.h"
#include "parse.h"

typedef struct CompileState CompileState;
typedef struct InstructionStack InstructionStack;
typedef struct LiteralValue LiteralValue;

struct InstructionStack {
	char* instructions[16]; /* 16 is way too high but better safe */
	unsigned int nins; /* number of instructions so far */
	unsigned int depth;
	InstructionStack* next;
	InstructionStack* prev;
};

struct LiteralValue {
	char* name;
	LiteralValue* next;	
	LiteralValue* prev;
	int is_c;
};

struct CompileState {
	TreeNode* node_focus;
	TreeNode* current_function;
	TreeBlock* root_block;
	LiteralValue* defined_functions;
	LiteralValue* string_literals;
	InstructionStack* ins_stack;
	unsigned int label_count;
	unsigned int literal_count;
	unsigned int main_label;
	unsigned int depth; /* block depth */
	unsigned int return_label; /* current return label for function exit */
	unsigned int body_size; /* current size of function stack frame */
	FILE* output;
};	

void generate_bytecode(TreeBlock*, const char*);

#endif
