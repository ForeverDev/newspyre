#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "generate.h"

#define LABEL_FORMAT "__LABEL__%04d"
#define DEF_LABEL_FORMAT "__LABEL__%04d:\n"
#define JMP_LABEL_FORMAT "jmp " LABEL_FORMAT "\n"
#define JNZ_LABEL_FORMAT "jnz " LABEL_FORMAT "\n"
#define JZ_LABEL_FORMAT "jz " LABEL_FORMAT "\n"

typedef struct ExpressionNode ExpressionNode;
typedef struct ExpressionStack ExpressionStack;

/* only holds a single token */
struct ExpressionNode {
	ExpressionNode* next;
	unsigned int is_func;

	Token* token; /* only applicable if not is_func */

	const char* func_name; /* only applicable if is_func */
	ExpressionNode* argument; /* only applicable if is_func */
};

struct ExpressionStack {
	ExpressionNode* node;
	ExpressionStack* next;
	ExpressionStack* prev;
};

static ExpressionNode* ts_pop(ExpressionStack*);
static ExpressionNode* ts_top(ExpressionStack*);
static void ts_push(ExpressionStack*, ExpressionNode*);
static unsigned int ts_length(ExpressionStack*);
static void writestr(CompileState*, const char*, ...);
static inline void writelabel(CompileState*);
static void compile_if(CompileState*);
static void compile_function(CompileState*);
static void generate_expression(CompileState*, ExpressionNode*);
static void node_advance(CompileState*);
static ExpressionNode* compile_expression(Token*);
static ExpressionNode* compile_function_call(Token**);
static void push_instruction(CompileState*, const char*, ...);
static void pop_instruction(CompileState*);
static int function_exists(CompileState*, const char*);
static void scan_for_calls(CompileState*, Token* expression);

static void
scan_for_calls(CompileState* state, Token* expression) {
	Token* at = expression;
	while (at->next) {
		if (at->type == TYPE_IDENTIFIER && at->next->type == TYPE_OPENPAR) {
			if (!function_exists(state, at->word)) {
				writestr(state, "let __CFUNC__%s \"%s\"\n", at->word, at->word);
				/* TODO FIX MEMORY LEAK! */
				char* save = at->word;
				at->word = malloc(32);
				sprintf(at->word, "__CFUNC__%s", save);
			}
		}
		at = at->next;
	}
}

static void
scan_for_literals(CompileState* state, Token* expression) {
	Token* at = expression;
	while (at) {
		if (at->type == TYPE_STRING) {
			/* TODO don't redefine literals that already exist */
			writestr(state, "let __STR__%04d \"%s\"\n", state->literal_count, at->word); 
			/* TODO FIX MEMORY LEAK! */
			at->word = malloc(32);
			sprintf(at->word, "__STR__%04d", state->literal_count); 
			state->literal_count++;
		}
		at = at->next;
	}
}

static int
function_exists(CompileState* state, const char* name) {
	LiteralValue* func = state->defined_functions;
	while (func) {
		if (!strcmp(func->name, name)) {
			return 1;
		}
		func = func->next;
	}
	return 0;
}

static void
push_instruction(CompileState* state, const char* instruction, ...) {
	InstructionStack* at = state->ins_stack;
	va_list list;
	va_start(list, instruction);
	if (!at) {
		InstructionStack* stack = calloc(1, sizeof(InstructionStack));
		stack->instructions[0] = malloc(64);
		vsprintf(stack->instructions[0], instruction, list);
		stack->nins = 1;
		stack->depth = state->depth;
		stack->next = NULL;
		stack->prev = NULL;
		state->ins_stack = stack;
	} else {
		while (at->next) {
			at = at->next;
		}
		/* append to this stack */
		if (at->depth == state->depth) {
			if (!at->instructions[at->nins]) {
				at->instructions[at->nins] = malloc(64);
			}	
			vsprintf(at->instructions[at->nins++], instruction, list);
		/* make a new stack */
		} else {
			InstructionStack* stack = calloc(1, sizeof(InstructionStack));
			stack->instructions[0] = malloc(64);
			vsprintf(stack->instructions[0], instruction, list);
			stack->nins = 1;
			stack->depth = state->depth;
			stack->next = NULL;
			stack->prev = at;
			at->next = stack;
		}
	}
	va_end(list);
}

static void
pop_instruction(CompileState* state) {
	if (!state->ins_stack) return;
	InstructionStack* at = state->ins_stack;
	while (at->next) {
		at = at->next;
	}
	for (int i = 0; i < at->nins; i++) {
		writestr(state, at->instructions[i]);	
		free(at->instructions[i]);
	}
	if (at->prev) {
		at->prev->next = NULL;
	} else {
		state->ins_stack = NULL;
	}
	free(at);
}

static void
writestr(CompileState* state, const char* format, ...) {
	va_list list;
	va_start(list, format);
	vfprintf(state->output, format, list);
	va_end(list);
}

static inline void
writelabel(CompileState* state) {
	writestr(state, DEF_LABEL_FORMAT, state->label_count++);
}

/* RETURN TYPES
	0: N/A
	1: ADVANCED
	2: JUMPED INTO
	3: JUMPED OUT OF ZERO
	4: JUMPED OUT OF ONE
	5: ...
*/
static void 
node_advance(CompileState* S) {
	if (S->node_focus->block && !S->node_focus->block->children) {
		pop_instruction(S);
	}
	/* there is a block to jump into */
	if (S->node_focus->block && S->node_focus->block->children) {
		S->node_focus = S->node_focus->block->children;
		S->depth++;
	/* there is another node in the current block */
	} else if (S->node_focus->next) {
		S->node_focus = S->node_focus->next;
	/* jump out of block(s) */
	} else {
		while (S->node_focus->parent_block && !S->node_focus->next) {
			S->node_focus = S->node_focus->parent_block->parent_node;
			S->depth--;
			pop_instruction(S);
		}
		if (S->node_focus->next) {
			S->node_focus = S->node_focus->next;
		}
	}
}

static void
ts_push(ExpressionStack* stack, ExpressionNode* node) {
	if (!stack->node) {
		stack->node = node;
		stack->next = NULL;
		stack->prev = NULL;
		return;
	}
	ExpressionStack* new = malloc(sizeof(ExpressionStack));
	new->node = node;
	ExpressionStack* at = stack;
	while (at->next) {
		at = at->next;
	}
	at->next = new;
	new->prev = at;
	new->next = NULL;
}

static ExpressionNode*
ts_pop(ExpressionStack* stack) {
	if (!stack->node) {
		return NULL;
	}
	ExpressionStack* at = stack;
	while (at->next) {
		at = at->next;
	}
	ExpressionNode* ret = at->node;
	if (at->prev) {
		at->prev->next = NULL;
		at->prev = NULL;
		free(at);
	} else {
		stack->node = NULL;
	}
	return ret;
}

static ExpressionNode*
ts_top(ExpressionStack* stack) {
	ExpressionStack* at = stack;
	while (at->next) {
		at = at->next;
	}
	return at->node;
}

static unsigned int
ts_length(ExpressionStack* stack) {
	unsigned int count = 0;
	if (!stack->node) {
		return 0;
	}
	ExpressionStack* at = stack;
	while (at) {
		at = at->next;
		count++;
	}
	return count;
}

static ExpressionNode*
compile_function_call(Token** expression) {
	
	/* starts on name of function */
	Token* at = *expression;
	Token* arg_start = at;

	/* expects to be on the first token of the arguments */
	ExpressionNode* node = malloc(sizeof(ExpressionNode));
	node->next = NULL;
	node->is_func = 1;
	node->func_name = at->word;
	node->token = malloc(sizeof(Token));
	node->token->type = TYPE_FUNCCALL;
	node->token->word = "FUNCTION_CALL";
	node->argument = NULL;
	at = at->next->next;
	if (at->type == TYPE_CLOSEPAR) {
		*expression = at ? at->next : NULL;
		return node;
	}
	node->argument = malloc(sizeof(ExpressionNode));
	node->argument->next = NULL;

	arg_start = at;

	int has_seen_par = 0;
	int count = 1;
	
	while (!has_seen_par || count > 0) {
		if (at->type == TYPE_OPENPAR) {
			has_seen_par = 1;
			count++;
		} else if (at->type == TYPE_CLOSEPAR) {
			if (!has_seen_par) break;
			count--;
			if (count == 0) break;
		}
		at = at->next;
	}

	Token* end = at;
	Token* next_save = end->next;
	end->next = NULL; /* detach from the list for now */
	*expression = end;
	
	ExpressionNode* compiled = compile_expression(arg_start);	
	ExpressionNode* counter = compiled;
	while (counter) {
		counter = counter->next;
	}
	end->next = next_save;
	node->argument = compiled;

	return node;

}

/* FIXME remove operator and postfix memory leak */
static ExpressionNode*
compile_expression(Token* expression) {
	/* now to apply shunting yard to the linked list of tokens

	   these token stacks are used only to rearrange the tokens
	   to be returned from the function
	*/
	ExpressionStack* operators = calloc(1, sizeof(ExpressionStack));
	ExpressionStack* postfix = calloc(1, sizeof(ExpressionStack));
	Token* at = expression;

	static const unsigned int op_pres[256] = {
		[TYPE_COMMA]		= 1,
		[TYPE_ASSIGN]		= 2,
		[TYPE_EQ]			= 3,
		[TYPE_NOTEQ]		= 3,
		[TYPE_PERIOD]		= 4,
		[TYPE_LOGAND]		= 4,
		[TYPE_LOGOR]		= 4,
		[TYPE_GT]			= 5,
		[TYPE_GE]			= 5,
		[TYPE_LT]			= 5,
		[TYPE_LE]			= 5,
		[TYPE_AMPERSAND]	= 6,
		[TYPE_LINE]			= 6,
		[TYPE_XOR]			= 6,
		[TYPE_SHL]			= 6,
		[TYPE_SHR]			= 6,
		[TYPE_PLUS]			= 7,
		[TYPE_HYPHON]		= 7,
		[TYPE_ASTER]		= 8,
		[TYPE_FORSLASH]		= 8
	};

	while (at) {
		if (at->next && at->type == TYPE_IDENTIFIER && at->next->type == TYPE_OPENPAR) {
			ExpressionNode* node = compile_function_call(&at); 
			ts_push(postfix, node);
		} else if (at->type == TYPE_IDENTIFIER || at->type == TYPE_NUMBER || at->type == TYPE_STRING) {
			ExpressionNode* node = calloc(1, sizeof(ExpressionNode));
			node->token = at;
			ts_push(postfix, node);
		} else if (at->type == TYPE_OPENPAR) {
			ExpressionNode* node = calloc(1, sizeof(ExpressionNode));
			node->token = at;
			ts_push(operators, node);
		} else if (op_pres[at->type]) {
			while (ts_length(operators) > 0 
				   && ts_top(operators)->token->type != TYPE_OPENPAR 
				   && op_pres[at->type] <= op_pres[ts_top(operators)->token->type]
			) {
				ts_push(postfix, ts_pop(operators));					
			}
			ExpressionNode* node = calloc(1, sizeof(ExpressionNode));
			node->token = at;
			ts_push(operators, node);
		} else if (at->type == TYPE_CLOSEPAR) {
			while (ts_length(operators) > 0 && ts_top(operators)->token->type != TYPE_OPENPAR) {
				ts_push(postfix, ts_pop(operators));	
			}
			ts_pop(operators);
		} else {
			printf("unknown token %s\n", at->word);
		}
		at = at->next;
	}
	
	/* TODO free postfix and operators properly */

	while (ts_length(operators) > 0) {
		ts_push(postfix, ts_pop(operators));
	}
	
	ExpressionNode* ret = postfix->node;
	
	/*	TOKENS ARE NOW STRUNG TOGETHER BY NODES!
		token->next is no longer a viable way to reach
		the next token!
	*/	
	while (postfix) {
		postfix->node->token->next = NULL;
		postfix->node->token->prev = NULL;
		postfix->node->next = postfix->next ? postfix->next->node : NULL;
		postfix = postfix->next;
	}

	return ret;

}

static void
generate_expression(CompileState* S, ExpressionNode* expression) {
	ExpressionNode* at = expression;
	while (at) {
		if (at->is_func) {
			/* args pushed in foward order, must be reverse popped by callee */
			unsigned int numargs = 0;
			if (at->argument) {
				numargs++;
				ExpressionNode* counter = at->argument;
				while (counter) {
					if (counter->token->type == TYPE_COMMA) {
						numargs++;
					}
					counter = counter->next;
				}
				generate_expression(S, expression->argument);	
			}
			if (function_exists(S, expression->func_name)) {
				writestr(S, "call %s, %d\n", expression->func_name, numargs);
			} else {
				writestr(S, "ccall %s\n", expression->func_name);
			}
		} else {
			switch (at->token->type) {
				case TYPE_NUMBER: writestr(S, "ipush %s\n", at->token->word); break;
				case TYPE_PLUS: writestr(S, "iadd\n"); break;
				case TYPE_HYPHON: writestr(S, "isub\n"); break;
				case TYPE_ASTER: writestr(S, "imul\n"); break;
				case TYPE_FORSLASH: writestr(S, "idiv\n"); break;
				case TYPE_LT: writestr(S, "ilt\n"); break;
				case TYPE_LE: writestr(S, "ile\n"); break;
				case TYPE_GT: writestr(S, "igt\n"); break;
				case TYPE_GE: writestr(S, "ige\n"); break;
				case TYPE_COMMA: break;
				case TYPE_STRING: 
					writestr(S, "ipush %s\n", at->token->word); 
					break;
				case TYPE_ASSIGN: {
					
					break;
				}
			}
		}
		at = at->next;
	}
}

static void
compile_if(CompileState* S) {
	push_instruction(S, DEF_LABEL_FORMAT, S->label_count);
	generate_expression(S, compile_expression(S->node_focus->words->token));
	writestr(S, JZ_LABEL_FORMAT, S->label_count);
	S->label_count++;
}

static void
compile_while(CompileState* S) {
	unsigned int start_label, finish_label;
	start_label = S->label_count++;
	finish_label = S->label_count++;
	writestr(S, DEF_LABEL_FORMAT, start_label);
	generate_expression(S, compile_expression(S->node_focus->words->token));
	writestr(S, JZ_LABEL_FORMAT, finish_label);
	push_instruction(S, JMP_LABEL_FORMAT, start_label);
	push_instruction(S, DEF_LABEL_FORMAT, finish_label);
}

static void
compile_function(CompileState* S) {
}

static void
compile_statement(CompileState* S) {
	generate_expression(S, compile_expression(S->node_focus->words->token));
}

void
generate_bytecode(TreeBlock* tree, const char* output_name) {
	CompileState* S = malloc(sizeof(CompileState));
	S->root_block = tree;
	S->node_focus = S->root_block->children;
	S->label_count = 0;
	S->literal_count = 0;
	S->depth = 0;
	S->output = fopen(output_name, "w");
	S->ins_stack = NULL;
	S->defined_functions = NULL;
	S->string_literals = NULL;
	if (!S->output) {
		printf("couldn't open file for writing\n");
		exit(1);
	}
	/* walk the tree once to find C functions, string literals, globals */
	while (S->node_focus) {
		if (S->node_focus->type == FUNCTION) {
			LiteralValue* func = malloc(sizeof(LiteralValue));
			func->name = S->node_focus->words->token->word;
			func->next = NULL;
			if (!S->defined_functions) {
				S->defined_functions = func;
			} else {
				LiteralValue* at = S->defined_functions;
				while (at->next) {
					at = at->next;
				}
				at->next = func;
			}
		} else if (
			S->node_focus->type == WHILE 
			|| S->node_focus->type == IF
			|| S->node_focus->type == STATEMENT
		) {
			scan_for_calls(S, S->node_focus->words->token);
			scan_for_literals(S, S->node_focus->words->token);
		}
		node_advance(S);
		if (!S->node_focus || S->node_focus->type == ROOT) {
			break;
		}	
	}
	S->node_focus = S->root_block->children;

	/* now generate */
	while (S->node_focus) {
		switch (S->node_focus->type) {
			case IF: compile_if(S); break;	
			case WHILE: compile_while(S); break;
			case FUNCTION: compile_function(S); break;
			default: compile_statement(S); break;
		}
		node_advance(S);
		if (!S->node_focus || S->node_focus->type == ROOT) {
			break;
		}	
	}
	while (S->ins_stack) {
		pop_instruction(S);
	}
	
	/* replace with something else? prevents the error that happens when
	 * a label isn't followed by any instructions */	
	writestr(S, "ipush 0\n");

	fclose(S->output);

}
