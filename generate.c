#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include "generate.h"

#define LABEL_FORMAT "__LABEL__%04d"
#define DEF_LABEL_FORMAT "__LABEL__%04d:"
#define JMP_LABEL_FORMAT "jmp " LABEL_FORMAT
#define JNZ_LABEL_FORMAT "jnz " LABEL_FORMAT
#define JZ_LABEL_FORMAT "jz " LABEL_FORMAT
#define COMMENT(s) " ; ----> " s "\n"

typedef struct ExpressionNode ExpressionNode;
typedef struct ExpressionStack ExpressionStack;

/* only holds a single token */
struct ExpressionNode {
	ExpressionNode* next;
	unsigned int is_func;

	Token* token; /* only applicable if not is_func */

	char* func_name; /* only applicable if is_func */
	TreeNode* fptr;
	char* datatype;
	ExpressionNode* argument; /* only applicable if is_func */
};

struct ExpressionStack {
	ExpressionNode* node;
	ExpressionStack* next;
	ExpressionStack* prev;
};

static void tc_push_inline(TypecheckObject*, char*);
static void tc_push_var(TypecheckObject*, TreeVariable*);
static void tc_append(TypecheckObject*, TypecheckObject*);
static TypecheckObject* tc_pop(TypecheckObject*);
static TypecheckObject* tc_top(TypecheckObject*);
static ExpressionNode* ts_pop(ExpressionStack*);
static ExpressionNode* ts_top(ExpressionStack*);
static void ts_push(ExpressionStack*, ExpressionNode*);
static unsigned int ts_length(ExpressionStack*);
static void writestr(CompileState*, const char*, ...);
static inline void writelabel(CompileState*);
static void compile_if(CompileState*);
static void compile_function_body(CompileState*);
static void compile_array_index(CompileState*, Token**);
static TypecheckObject* generate_expression(CompileState*, ExpressionNode*);
static void compile_return(CompileState*);
static void compile_assignment(CompileState*);
static ExpressionNode* compile_expression(CompileState*, Token*);
static ExpressionNode* compile_function_call(CompileState*, Token**);
static void push_instruction(CompileState*, const char*, ...);
static void pop_instruction(CompileState*);
static int function_exists(CompileState*, const char*);
static void scan_for_calls(CompileState*, Token* expression);
static void advance(CompileState*);
static unsigned int count_function_var_size(CompileState*);
static unsigned int count_function_args(CompileState*);
static TreeVariable* find_variable(CompileState*, const char*);
static void comp_error(CompileState*, const char*, ...);

static void
comp_error(CompileState* S, const char* format, ...) {
	va_list list;
	va_start(list, format);
	
	printf("\n\n----> SPYRE COMPILE-TIME ERROR (LINE %d) <----\n\n", S->node_focus->line);
	vprintf(format, list);
	printf("\n\n\n");
	va_end(list);

	exit(1);
}

static void
tc_append(TypecheckObject* obj, TypecheckObject* new) {
	/* datatype won't exist if obj hasn't been assigned */
	if (!obj->datatype) {
		memcpy(obj, new, sizeof(TypecheckObject));
		return;
	}
	TypecheckObject* i;
	for (i = obj; i->next; i = i->next);
	i->next = new;
	new->next = NULL;
	new->prev = i;
}

static TypecheckObject*
tc_pop(TypecheckObject* obj) {
	if (!obj->datatype) return NULL;
	TypecheckObject* i;
	for (i = obj; i->next; i = i->next);
	if (i->prev) {
		i->prev->next = NULL;
		i->prev = NULL;
	}
	return i;
}

static void
tc_push_inline(TypecheckObject* obj, char* type) {
	TypecheckObject* new = calloc(1, sizeof(TypecheckObject));
	new->datatype = type;
	tc_append(obj, new);
}

static void
tc_push_var(TypecheckObject* obj, TreeVariable* var) {
	TypecheckObject* new = calloc(1, sizeof(TypecheckObject));
	new->datatype = var->datatype;
	new->is_var = 1;
	new->offset = var->offset;
	new->identifier = var->identifier;
	tc_append(obj, new);
}

static TreeVariable*
find_variable(CompileState* S, const char* identifier) {
	TreeBlock* block = S->node_focus->parent_block;
	while (block) {
		TreeVariable* var = block->locals;
		while (var) {
			if (!strcmp(var->identifier, identifier)) {
				return var;
			}
			var = var->next;
		}
		if (!block->parent_node) break;
		block = block->parent_node->parent_block;
	}
	comp_error(S, 
		"undeclared identifier %s", 
		identifier, 
		S->current_function
	);
	return NULL;
}

static unsigned int
count_function_var_size(CompileState* S) {
	unsigned int size = 0;
	/* TODO recursive call into the body's inner blocks to find
	 * nested variable declarations */
	TreeVariable* at = S->node_focus->block->locals;
	while (at) {
		if (!at->is_arg && strcmp(at->identifier, "__RETURN_TYPE__")) {
			size += at->size;
		}
		at = at->next;
	}
	return size;
}

static unsigned int
count_function_args(CompileState* S) {
	unsigned int num = 0;
	return 0;
}

static void
scan_for_calls(CompileState* S, Token* expression) {
	Token* at = expression;
	while (at->next) {
		if (at->type == TYPE_IDENTIFIER && at->next->type == TYPE_OPENPAR) {
			if (!function_exists(S, at->word)) {
				writestr(S, "let __CFUNC__%s \"%s\"\n", at->word, at->word);
				LiteralValue* used_mark = malloc(sizeof(LiteralValue));
				used_mark->name = at->word;
				used_mark->next = NULL;
				used_mark->is_c = 1;
				LiteralValue* head = S->defined_functions;
				while (head->next) head = head->next;
				head->next = used_mark;
			}
		}
		at = at->next;
	}
}

static void
scan_for_literals(CompileState* S, Token* expression) {
	Token* at = expression;
	while (at) {
		if (at->type == TYPE_STRING) {
			/* TODO don't redefine literals that already exist */
			writestr(S, "let __STR__%04d \"%s\"\n", S->literal_count, at->word); 
			/* TODO FIX MEMORY LEAK! */
			at->word = malloc(32);
			sprintf(at->word, "__STR__%04d", S->literal_count); 
			S->literal_count++;
		}
		at = at->next;
	}
}

static int
function_exists(CompileState* S, const char* name) {
	LiteralValue* func = S->defined_functions;
	while (func) {
		if (!strcmp(func->name, name)) {
			return func->is_c ? 2 : 1;
		}
		func = func->next;
	}
	return 0;
}

static void
push_instruction(CompileState* S, const char* instruction, ...) {
	InstructionStack* at = S->ins_stack;
	va_list list;
	va_start(list, instruction);
	if (!at) {
		InstructionStack* stack = calloc(1, sizeof(InstructionStack));
		stack->instructions[0] = malloc(64);
		vsprintf(stack->instructions[0], instruction, list);
		stack->nins = 1;
		stack->depth = S->depth;
		stack->next = NULL;
		stack->prev = NULL;
		S->ins_stack = stack;
	} else {
		while (at->next) {
			at = at->next;
		}
		/* append to this stack */
		if (at->depth == S->depth) {
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
			stack->depth = S->depth;
			stack->next = NULL;
			stack->prev = at;
			at->next = stack;
		}
	}
	va_end(list);
}

static void
pop_instruction(CompileState* S) {
	if (!S->ins_stack) return;
	InstructionStack* at = S->ins_stack;
	while (at->next) {
		at = at->next;
	}
	for (int i = 0; i < at->nins; i++) {
		writestr(S, at->instructions[i]);	
		free(at->instructions[i]);
	}
	if (at->prev) {
		at->prev->next = NULL;
	} else {
		S->ins_stack = NULL;
	}
	free(at);
}

static void
writestr(CompileState* S, const char* format, ...) {
	va_list list;
	va_start(list, format);
	vfprintf(S->output, format, list);
	va_end(list);
}

static inline void
writelabel(CompileState* S) {
	writestr(S, DEF_LABEL_FORMAT, S->label_count++);
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
advance(CompileState* S) {
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

static void
compile_array_index(CompileState* S, Token** expression) {
}

static ExpressionNode*
compile_function_call(CompileState* S, Token** expression) {
	
	/* starts on name of function */
	Token* at = *expression;
	Token* arg_start = at;

	/* find the function declaration */
	char* datatype = NULL;
	TreeNode* fptr = NULL;
	for (TreeNode* i = S->root_block->children; i; i = i->next) {
		if (i->type == FUNCTION && i->ret) {
			if (!strcmp(i->words->token->word, at->word)) {
				datatype = i->ret->datatype;
				fptr = i;
			}
		}
	}		
	/* this is TERRIBLE, but for the time being, assume it is
	 * an integer that is returned from the function
	 */
	if (!datatype) {
		datatype = "int";
	}

	/* expects to be on the first token of the arguments */
	ExpressionNode* node = malloc(sizeof(ExpressionNode));
	node->datatype = datatype;
	node->next = NULL;
	node->is_func = 1;
	node->func_name = at->word;
	node->token = malloc(sizeof(Token));
	node->token->type = TYPE_FUNCCALL;
	node->token->word = "FUNCTION_CALL";
	node->argument = NULL;
	node->fptr = fptr;
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
	
	ExpressionNode* compiled = compile_expression(S, arg_start);	
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
compile_expression(CompileState* S, Token* expression) {
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
		[TYPE_LOGAND]		= 2,
		[TYPE_LOGOR]		= 2,
		[TYPE_EQ]			= 3,
		[TYPE_NOTEQ]		= 3,
		[TYPE_PERIOD]		= 4,
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
		[TYPE_PERCENT]		= 8,
		[TYPE_FORSLASH]		= 8
	};

	while (at) {
		if (at->next && at->type == TYPE_IDENTIFIER && at->next->type == TYPE_OPENPAR) {
			ExpressionNode* node = compile_function_call(S, &at); 
			ts_push(postfix, node);
		} else if (at->type == TYPE_IDENTIFIER || at->type == TYPE_NUMBER || at->type == TYPE_STRING) {
			ExpressionNode* node = calloc(1, sizeof(ExpressionNode));
			node->token = at;
			node->datatype = (
				at->type == TYPE_IDENTIFIER ? find_variable(S, at->word)->datatype :
				at->type == TYPE_NUMBER ? "int" :
				at->type == TYPE_STRING ? "string" : "__NA__"
			);
			ts_push(postfix, node);
		} else if (at->type == TYPE_OPENPAR) {
			ExpressionNode* node = calloc(1, sizeof(ExpressionNode));
			node->token = at;
			node->datatype = "__NA__";
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
			node->datatype = "__NA__";
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
	while (postfix && postfix->node) {
		postfix->node->token->next = NULL;
		postfix->node->token->prev = NULL;
		postfix->node->next = postfix->next ? postfix->next->node : NULL;
		postfix = postfix->next;
	}

	return ret;

}

static TypecheckObject* 
generate_expression(CompileState* S, ExpressionNode* expression) {
	ExpressionNode* at = expression;
	TypecheckObject* types = calloc(1, sizeof(TypecheckObject));
	if (!at) {
		types->datatype = "null";
		return types;
	}
	while (at) {
		if (at->is_func) {
			/* args pushed in foward order, must be reverse popped by callee */
			unsigned int numargs = 0;
			if (at->argument) {
				numargs++;
				ExpressionNode* counter = at->argument;
				/* find the number of args */
				while (counter) {
					if (counter->token->type == TYPE_COMMA) {
						numargs++;
					}
					counter = counter->next;
				}
				/* for now don't typecheck C functions... eventually some sort of "extern" statement
				   will be needed for those
				*/
				TypecheckObject* param_types = generate_expression(S, at->argument);
				if (at->fptr) {
					TreeVariable* params = at->fptr->block->locals;
					unsigned int index = 0;
					unsigned int nargs = 0;
					for (TypecheckObject* i = param_types; i; i = i->next) {
						nargs++;
					}
					for (TypecheckObject* i = param_types; i && params; i = i->next) {
						if (!strcmp(i->datatype, "int") && !strcmp(params->datatype, "float")) {
							/* attempt to pass int to parameter of type float, implicit cast */
							writestr(S, "itof %d", nargs - index - 1);
							writestr(S, COMMENT("implicit cast int->float @ARG[%d]"), index);
						} else if (!strcmp(i->datatype, "float") && !strcmp(params->datatype, "int")) {
							/* attempt to pass float to parameter of type int, implicit cast */
							writestr(S, "ftoi %d", nargs - index - 1);
							writestr(S, COMMENT("implicit cast float->int @ARG[%d]"), index);
						} else if (strcmp(i->datatype, params->datatype)) {
							comp_error(S,
								"parameter #%d is of type (%s). expected type (%s)",
								index + 1, 
								i->datatype, 
								params->datatype
							);
						}
						index++;
						params = params->next;
					}
				}
			}
			int result_func = function_exists(S, at->func_name);
			if (result_func == 1) {
				writestr(S, "call __FUNC__%s, %d\n", at->func_name, numargs);
				tc_push_inline(types, at->datatype);
			} else if (result_func == 2) {
				writestr(S, "ccall __CFUNC__%s, %d\n", at->func_name, numargs);
				tc_push_inline(types, "int");
			}
		} else {
			/* TYPECHECKING TIME!
			 * only typecheck certain operators.. e.g plus, minus, multiply,
			 * divide, etc.  datatypes are on the 'types' stack.
			 * implicit casting (e.g. (50.0 * 3) -> (50.0 * 3.0)) is done
			 * in this phase as well
			 */
			TypecheckObject* pop[2] = {0};
			int isfloat[2] = {0};
			int isint[2] = {0};
			switch (at->token->type) {
				case TYPE_PLUS:
				case TYPE_HYPHON:
				case TYPE_ASTER:
				case TYPE_FORSLASH:
				case TYPE_LT:
				case TYPE_LE:
				case TYPE_GT:
				case TYPE_GE:
				case TYPE_EQ:
					for (int i = 0; i < 2; i++) {
						pop[i] = tc_pop(types);
						isfloat[i] = !strcmp(pop[i]->datatype, "float");
						if (!isfloat[i]) {
							isint[i] = !strcmp(pop[i]->datatype, "int");
						}
					}
					if (!pop[0] || !pop[1]) {
						comp_error(S, "malformed expression");
					}
					if ((isfloat[0] && isint[1]) || (isfloat[1] && isint[0])) {
						/* arithmetic is being done between a float
						 * and an int... e.g. (50.0 * 3).  convert it
						 * to an expression with two floats
						 */
						tc_push_inline(types, "float");
						writestr(S, "itof %d", isfloat[0]);
						writestr(S, COMMENT("implicit cast int->float @[SP - %d]"), isfloat[0]);
						writestr(S, "f");
					} else if (strcmp(pop[0]->datatype, pop[1]->datatype)) {
						comp_error(S,
							"attempt to perform arithmetic on two incompatible types: (%s) and (%s)",			
							pop[0]->datatype,
							pop[1]->datatype
						);
					} else {
						tc_push_inline(types, pop[0]->datatype);
						/* write the prefix to the next instruction */
						writestr(S, !strcmp(pop[0]->datatype, "float") ? "f" : "i");
					}
					break;
			}
			switch (at->token->type) {
				case TYPE_PLUS: 
					writestr(S, "add\n"); 
					break;
				case TYPE_HYPHON: 
					writestr(S, "sub\n"); 
					break;
				case TYPE_ASTER: 
					writestr(S, "mul\n"); 
					break;
				case TYPE_FORSLASH: 
					writestr(S, "div\n"); 
					break;
				case TYPE_LT: 
					writestr(S, "lt\n"); 
					break;
				case TYPE_LE: 
					writestr(S, "le\n"); 
					break;
				case TYPE_GT: 
					writestr(S, "gt\n"); 
					break;
				case TYPE_GE:
					writestr(S, "ge\n"); 
					break;
				case TYPE_EQ: 
					writestr(S, "cmp\n"); 
					break;
				case TYPE_PERCENT: 
					writestr(S, "mod\n"); 
					break;
				case TYPE_SHR: 
					writestr(S, "shr\n"); 
					break;
				case TYPE_SHL: 
					writestr(S, "shl\n"); 
					break;
				case TYPE_LOGOR: 
					writestr(S, "lor\n"); 
					break;
				case TYPE_LOGAND: 
					writestr(S, "land\n"); 
					break;
				case TYPE_COMMA: 
					break;
				case TYPE_NUMBER: { 
					/* scan for a decimal to check if it's a float */
					for (char* i = at->token->word; *i; i++) {
						if (*i == '.') {
							tc_push_inline(types, "float");	
							writestr(S, "fpush %s\n", at->token->word);
							goto number_done;
						}
					}
					tc_push_inline(types, "int");
					writestr(S, "ipush %s\n", at->token->word); 
					number_done:
					break;
				}
				case TYPE_STRING: 
					tc_push_inline(types, "string");
					writestr(S, "ipush %s\n", at->token->word); 
					break;
				case TYPE_ASSIGN: {
					
					break;
				}
				case TYPE_IDENTIFIER: {
					TreeVariable* var = find_variable(S, at->token->word);
					tc_push_var(types, var);
					if (!strcmp(var->datatype, "float")) {
						writestr(S, "flload %d", var->offset);
					} else {
						writestr(S, "ilload %d", var->offset);
					}
					writestr(S, COMMENT("%s"), var->identifier);
					break;
				}
			}
		}
		at = at->next;
	}

	return types;
}

static void
compile_for(CompileState* S) {
}

static void 
compile_assignment(CompileState* S) {
	TypecheckObject* t = generate_expression(S, compile_expression(S, S->node_focus->words->next->token));
	t = tc_pop(t);
	/* no structs yet, should only be a single var name */
	TreeVariable* local = find_variable(S, S->node_focus->words->token->word);
	if (!strcmp(local->datatype, "float") && !strcmp(t->datatype, "int")) {
		/* implicit cast to int */
		writestr(S, "itof 0 %s", COMMENT("implicit cast float->int ASSIGNMENT"));
	} else if (!strcmp(local->datatype, "int") && !strcmp(t->datatype, "float")) {
		writestr(S, "ftoi 0 %s", COMMENT("implicit cast int->float ASSIGNMENT"));
	} else if (strcmp(t->datatype, local->datatype)) {
		comp_error(S,
			"attempt to assign expression that results in type (%s) to variable (%s). expected type (%s)", 
			t->datatype, 
			local->identifier,
			local->datatype
		);
	}
	if (!strcmp(local->datatype, "float")) {
		writestr(S, "flsave %d", local->offset);
	} else {
		writestr(S, "ilsave %d", local->offset);
	}
	writestr(S, COMMENT("%s"), local->identifier);
}

static void
compile_if(CompileState* S) {
	push_instruction(S, DEF_LABEL_FORMAT, S->label_count);
	push_instruction(S, " ; if bottom\n");
	generate_expression(S, compile_expression(S, S->node_focus->words->token));
	writestr(S, JZ_LABEL_FORMAT, S->label_count);
	writestr(S, COMMENT("if condition jump"));
	S->label_count++;
}

static void
compile_while(CompileState* S) {
	unsigned int start_label, finish_label;
	start_label = S->label_count++;
	finish_label = S->label_count++;
	writestr(S, DEF_LABEL_FORMAT, start_label);
	writestr(S, COMMENT("while top"));
	ExpressionNode* exp = compile_expression(S, S->node_focus->words->token);
	generate_expression(S, exp);
	writestr(S, JZ_LABEL_FORMAT, finish_label);
	writestr(S, COMMENT("while condition jump"));
	push_instruction(S, JMP_LABEL_FORMAT, start_label);
	push_instruction(S, COMMENT("while jump top"));
	push_instruction(S, DEF_LABEL_FORMAT, finish_label);
	push_instruction(S, COMMENT("while bottom"));
}

static void
compile_function_body(CompileState* S) {
	unsigned int body_size = count_function_var_size(S);
	unsigned int return_label;
	writestr(S, "__FUNC__%s:\n", S->node_focus->words->token->word);
	S->return_label = S->label_count++;
	S->body_size = body_size;
	S->current_function = S->node_focus;
	for (TreeVariable* i = S->node_focus->block->locals; i; i = i->next) {
		if (i->is_arg) {
			writestr(S, "iarg %d\n", i->offset);
		}
	}
	writestr(S, "res %d\n", S->body_size); 
	push_instruction(S, DEF_LABEL_FORMAT, S->return_label);
	push_instruction(S, COMMENT("return label"));
	push_instruction(S, "iret\n");
}

static void
compile_return(CompileState* S) {
	TypecheckObject* t = generate_expression(S, compile_expression(S, S->node_focus->words->token));
	if (t->datatype && S->current_function->ret && S->current_function->ret->datatype) {
		if (strcmp(t->datatype, S->current_function->ret->datatype)) {
			comp_error(S,
				"attempt to return expression that results in type (%s). expected type (%s)", 
				t->datatype, 
				S->current_function->ret->datatype
			);
		}
	}
	writestr(S, JMP_LABEL_FORMAT, S->return_label); 
	writestr(S, COMMENT("return from %s"), S->current_function->words->token->word);
}

static void
compile_statement(CompileState* S) {
	ExpressionNode* expression = compile_expression(S, S->node_focus->words->token);
	generate_expression(S, expression);
}

void
generate_bytecode(TreeBlock* tree, const char* output_name) {
	CompileState* S = malloc(sizeof(CompileState));
	S->root_block = tree;
	S->current_function = NULL;
	S->node_focus = S->root_block->children;
	S->label_count = 0;
	S->main_label = 0;
	S->return_label = 0;
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
	/* walk the tree once to find C functions, string literals, (globals?) */
	while (S->node_focus) {
		if (S->node_focus->type == FUNCTION) {
			LiteralValue* func = malloc(sizeof(LiteralValue));
			func->is_c = 0;
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
		}
		advance(S);
		if (!S->node_focus || S->node_focus->type == ROOT) {
			break;
		}	
	}
	
	S->node_focus = S->root_block->children;	
	
	/* walk the tree again to find function calls */
	while (S->node_focus) {
		if (
			S->node_focus->type == WHILE 
			|| S->node_focus->type == IF
			|| S->node_focus->type == STATEMENT
			|| S->node_focus->type == RETURN
		) {
			scan_for_calls(S, S->node_focus->words->token);
			scan_for_literals(S, S->node_focus->words->token);
		}
		advance(S);
		if (!S->node_focus || S->node_focus->type == ROOT) {
			break;
		}	
	}
	S->node_focus = S->root_block->children;

	writestr(S, "jmp __ENTRY_POINT__\n");

	/* now generate */
	while (S->node_focus) {
		switch (S->node_focus->type) {
			case IF: compile_if(S); break;	
			case WHILE: compile_while(S); break;
			case FUNCTION: compile_function_body(S); break;
			case RETURN: compile_return(S); break;
			case ASSIGNMENT: compile_assignment(S); break;
			case DECLARATION: break;
			default: compile_statement(S); break;
		}
		advance(S);
		if (!S->node_focus || S->node_focus->type == ROOT) {
			break;
		}	
	}
	while (S->ins_stack) {
		pop_instruction(S);
	}
	
	/* replace with something else? prevents the error that happens when
	 * a label isn't followed by any instructions */	
	writestr(S, "__ENTRY_POINT__:\n");
	writestr(S, "call __FUNC__main, 0", S->main_label);

	fclose(S->output);

}
