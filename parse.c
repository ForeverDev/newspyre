#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "parse.h"

typedef struct TokenStack {
	Token* token;
	struct TokenStack* next;
	struct TokenStack* prev;
} TokenStack;

static void parse_if(SyntaxTree*);
static void parse_else(SyntaxTree*);
static void parse_while(SyntaxTree*);
static void parse_do_while(SyntaxTree*);
static void parse_function(SyntaxTree*);
static void parse_return(SyntaxTree*);
static void parse_switch(SyntaxTree*);
static void parse_case(SyntaxTree*);
static void parse_continue(SyntaxTree*);
static void parse_break(SyntaxTree*);
static void parse_for(SyntaxTree*);
static void handle_statement(SyntaxTree*);
static void parse_error(SyntaxTree*, const char*, ...);
static void jump_out(SyntaxTree*);
static void jump_in(SyntaxTree*, SyntaxBlock*);

static inline void append_token_copy(Token*, Token*);
static inline Token* parse_expression(SyntaxTree*);
static Token* parse_expression_count(SyntaxTree*, unsigned int, unsigned int);
static SyntaxNode* get_node_tail(SyntaxTree*);
static Token* copy_token(Token*);
static void print_block(SyntaxBlock*, unsigned int);
static void append_to_block(SyntaxTree*, SyntaxNode*);
static void append_node(SyntaxNode*, SyntaxNode*);
static SyntaxNode* new_node(SyntaxTree*, unsigned int, int);
static void append_word(SyntaxWord*, SyntaxWord*);

static void ts_push(TokenStack*, Token*);
static Token* ts_pop(TokenStack*);
static unsigned int ts_length(TokenStack*);
static Token* ts_index(TokenStack*, unsigned int);

static void
ts_push(TokenStack* stack, Token* token) {
	if (!stack->token) {
		stack->token = token;
		stack->next = NULL;
		stack->prev = NULL;
		return;
	}
	TokenStack* new = malloc(sizeof(TokenStack));
	new->token = token;
	TokenStack* at = stack;
	while (at->next) {
		at = at->next;
	}
	at->next = new;
	new->prev = at;
	new->next = NULL;
}

static Token*
ts_pop(TokenStack* stack) {
	if (!stack->token) {
		return NULL;
	}
	TokenStack* at = stack;
	while (at->next) {
		at = at->next;
	}
	Token* ret = at->token;
	if (at->prev) {
		at->prev->next = NULL;
		at->prev = NULL;
		free(at);
	} else {
		stack->token = NULL;
	}
	return ret;
}

static unsigned int
ts_length(TokenStack* stack) {
	unsigned int count = 0;
	if (!stack->token) {
		return 0;
	}
	TokenStack* at = stack;
	while (at) {
		at = at->next;
		count++;
	}
	return count;
}

static Token*
ts_index(TokenStack* stack, unsigned int index) {
	TokenStack* at = stack;
	while (at->next) {
		at = at->next;
	}
	for (int i = 0; i < index; i++) {
		at = at->prev;
	}
	return at->token;
}

static void 
append_word(SyntaxWord* head, SyntaxWord* word) {
	SyntaxWord* at = head;
	while (at->next) {
		at = at->next;
	}
	at->next = word;
	word->next = NULL;
}

static SyntaxNode*
new_node(SyntaxTree* T, unsigned int type, int has_block) {
	SyntaxNode* node = malloc(sizeof(SyntaxNode));
	node->type = type;
	node->words = malloc(sizeof(SyntaxWord));
	node->words->token = NULL;
	node->words->next = NULL;
	node->next = NULL;
	node->prev = NULL;
	node->block = has_block ? malloc(sizeof(SyntaxBlock)) : NULL;
	if (node->block) {
		node->block->node_parent = node;
		node->block->children = NULL;
	}
	node->block_parent = T->current_block;
	return node;
}

static void
parse_error(SyntaxTree* T, const char* format, ...) {
	va_list list;
	va_start(list, format);

	vprintf(format, list);
	printf("\nline: %d\n", T->tokens->line);
	va_end(list);

	exit(1);
}

static void
jump_out(SyntaxTree* T) {
	if (!T->current_block->node_parent) {
		printf("done with program\n");
		exit(0);	
	}
	T->current_block = T->current_block->node_parent->block_parent;
	T->tokens = T->tokens->next;
}

static void
jump_in(SyntaxTree* T, SyntaxBlock* block) {
	T->current_block = block;
}

static void
append_node(SyntaxNode* head, SyntaxNode* node) {
	SyntaxNode* at = head;
	if (at->type == 0) {
		at->type = node->type;
		at->words = node->words;
		at->next = NULL;
		at->prev = NULL;
		at->block = node->block;
		at->block_parent = node->block_parent;
	} else {
		while (at->next) {
			at = at->next;
		}
		at->next = node;
		node->prev = at;
		node->next = NULL;
	}
}

static void
append_to_block(SyntaxTree* T, SyntaxNode* node) {
	if (!T->current_block->children) {
		T->current_block->children = calloc(1, sizeof(SyntaxNode));
	}
	append_node(T->current_block->children, node);
}


static Token*
copy_token(Token* token) {
	Token* new = malloc(sizeof(Token));
	new->word = token->word;
	new->type = token->type;
	new->line = token->line;
	new->next = NULL;
	return new;
}

static SyntaxNode*
get_node_tail(SyntaxTree* T) {
	SyntaxNode* at = T->nodes;
	while (at->next) {
		at = at->next; 
	}	
	return at;
}

static inline void
append_token_copy(Token* head, Token* to_copy) {
	append_token(head, to_copy->word, to_copy->line, to_copy->type);
}

static inline Token*
parse_expression(SyntaxTree* T) {
	return parse_expression_count(T, 0, ';');
}

static Token*
parse_expression_count(SyntaxTree* T, unsigned int inc, unsigned int dec) {
	Token* expression = blank_token();	
	unsigned int count = 1;
	append_token_copy(expression, T->tokens);
	T->tokens = T->tokens->next;
	while (T->tokens) {
		if (T->tokens->type == inc) {
			count++;
		} else if (T->tokens->type == dec) {
			count--;
		}
		if (count == 0) {
			T->tokens = T->tokens->next;
			break;
		}
		append_token_copy(expression, T->tokens);
		T->tokens = T->tokens->next;
	}

	/* now to apply shunting yard to the linked list of tokens

	   these token stacks are used only to rearrange the tokens
	   to be returned from the function
	*/
	TokenStack* operators = calloc(1, sizeof(TokenStack));
	TokenStack* postfix = calloc(1, sizeof(TokenStack));
	Token* at = expression;

	static const unsigned int op_pres[256] = {
		[TYPE_COMMA]		= 1,
		[TYPE_EQ]			= 2,
		[TYPE_NOTEQ]		= 2,
		[TYPE_PERIOD]		= 3,
		[TYPE_LOGAND]		= 3,
		[TYPE_LOGOR]		= 3,
		[TYPE_GT]			= 4,
		[TYPE_GE]			= 4,
		[TYPE_LT]			= 4,
		[TYPE_LE]			= 4,
		[TYPE_AMPERSAND]	= 5,
		[TYPE_LINE]			= 5,
		[TYPE_XOR]			= 5,
		[TYPE_SHL]			= 5,
		[TYPE_SHR]			= 5,
		[TYPE_PLUS]			= 6,
		[TYPE_HYPHON]		= 6,
		[TYPE_ASTER]		= 7,
		[TYPE_FORSLASH]		= 7
	};

	while (at) {
		if (at->next && at->type == TYPE_IDENTIFIER && at->next->type == '(') {
			/* TODO PARSE FUNCTION CALL */
		} else if (at->type == TYPE_IDENTIFIER || at->type == TYPE_NUMBER || at->type == TYPE_STRING) {
			printf("found__ %s\n", at->word);
			ts_push(postfix, at);
		} else if (at->type == '(') {
			ts_push(operators, at);
		} else if (op_pres[at->type]) {
			printf("found %s\n", at->word);
			ts_length(operators);
			ts_index(operators, 0)->type;
			op_pres[at->type];
			op_pres[ts_index(operators, 0)->type];
			while (ts_length(operators) > 0 
				   && ts_index(operators, 0)->type != TYPE_OPENPAR 
				   && op_pres[at->type] <= op_pres[ts_index(operators, 0)->type]
			) {
				ts_push(postfix, ts_pop(operators));					
			}
			ts_push(operators, at);
		} else if (at->type == ')') {
			while (ts_length(operators) > 0 && ts_index(operators, 0)->type != TYPE_OPENPAR) {
				ts_push(postfix, ts_pop(operators));	
			}
			ts_pop(operators);
		} else {
			printf("unknown token %s\n", at->word);
		}
		at = at->next;
	}

	while (ts_length(operators) > 0) {
		ts_push(postfix, ts_pop(operators));
	}
	
	free(operators);

	while (postfix->next) {
		postfix->token->next = postfix->next->token;
		postfix = postfix->next;
	}
	postfix->token->next = NULL;

	free(postfix);

	return expression;
}

#define INDENT() for (int i = 0; i < indent; i++) printf("\t")

static void 
print_block(SyntaxBlock* block, unsigned int indent) {
	SyntaxNode* node = block->children;
	if (!node) {
		return;
	}
	SyntaxWord* word;
	while (node) {
		word = node->words;
		INDENT();
		printf("(%s", (
			node->type == STATEMENT ? "STATEMENT" :
			node->type == IF ? "IF" :
			node->type == ELSE ? "ELSE" :
			node->type == WHILE ? "WHILE" :
			node->type == DO_WHILE ? "DO_WHILE" :
			node->type == FUNCTION ? "FUNCTION" :
			node->type == RETURN ? "RETURN" :
			node->type == SWITCH ? "SWITCH" :
			node->type == CASE ? "CASE" :
			node->type == CONTINUE ? "CONTINUE" :
			node->type == BREAK ? "BREAK" :
			node->type == FOR ? "FOR" : "????"
		));
		if (!word) {
			printf(")");
		} else {
			printf("\n");
		}
		int word_count = 0;
		while (word && ++word_count) {
			INDENT();
			printf(" word %d: ", word_count - 1);
			Token* at = word->token;
			while (at) {
				printf("%s ", at->word);
				at = at->next;
			}
			printf("\n");
			word = word->next;
		}	
		INDENT();
		if (word_count > 0) {
			printf(")");
		}
		if (node->block) {
			printf(" {\n");
			print_block(node->block, indent + 1);
			INDENT();
			printf("}");
		}
		printf("\n");
		node = node->next;
	}	
}

void print_tree(SyntaxTree* T) {
	print_block(T->root_block, 0);	
}

SyntaxTree*
generate_tree(Token* tokens) {
	SyntaxTree* T = malloc(sizeof(SyntaxTree));
	T->tokens = tokens;
	T->nodes = malloc(sizeof(SyntaxNode));
	T->nodes->type = ROOT;
	T->nodes->next = NULL;
	T->nodes->prev = NULL;
	T->nodes->block = malloc(sizeof(SyntaxBlock));
	T->nodes->block->node_parent = T->nodes;
	T->nodes->block->children = NULL;
	T->nodes->block_parent = NULL;
	T->current_block = T->nodes->block;
	T->root_block = T->current_block;
	
	while (T->tokens) {
		switch (T->tokens->type) {
			case 1: parse_if(T); break;
			case 2: parse_else(T); break;
			case 3: parse_while(T); break;
			case 4: parse_do_while(T); break;
			case 5: parse_function(T); break;
			case 6: parse_return(T); break;
			case 7: parse_switch(T); break;
			case 8: parse_case(T); break;
			case 9: parse_continue(T); break;
			case 10: parse_break(T); break;
			case 11: parse_for(T); break;
			case '}': jump_out(T); break;
			default: handle_statement(T); break; 
		}
	}
	
	print_tree(T);

	return T;
}

static void
handle_statement(SyntaxTree* T) {
	Token* expression = parse_expression(T);
	SyntaxNode* node = new_node(T, STATEMENT, 0);
	node->words->token = expression;
	append_to_block(T, node);
}

static void
parse_if(SyntaxTree* T) {
	/* begins on token IF */
	T->tokens = T->tokens->next;
	/* now on first token of condition */
	if (T->tokens->type == '{') {
		parse_error(T, "Expected condition in if statement");	
	}
	SyntaxNode* node = new_node(T, IF, 1); 
	node->words->token = parse_expression_count(T, 0, '{');
	append_to_block(T, node);
	jump_in(T, node->block);
}

static void
parse_else(SyntaxTree* T) {
	/* begins on token ELSE */
	T->tokens = T->tokens->next;
	if (T->tokens->type != '{') {
		parse_error(T, "Expected '{' after else");
	}
	SyntaxNode* node = new_node(T, ELSE, 1);
	append_to_block(T, node);
	jump_in(T, node->block);
}

static void
parse_while(SyntaxTree* T) {
	/* begins on token WHILE */
	T->tokens = T->tokens->next;
	/* now on first token of condition */
	if (T->tokens->type == '{') {
		parse_error(T, "Expected condition in while loop");	
	}
	SyntaxNode* node = new_node(T, WHILE, 1); 
	node->words->token = parse_expression_count(T, 0, '{');
	append_to_block(T, node);
	jump_in(T, node->block);
}

static void
parse_do_while(SyntaxTree* T) {

}

/* syntax function name(arg_name : arg_type, arg_name : arg_type) -> return_type { ... } */
static void
parse_function(SyntaxTree* T) {
	/* begins on token FUNCTION */
	T->tokens = T->tokens->next;
	/* now on name of function */
	SyntaxNode* node = new_node(T, FUNCTION, 1);
	SyntaxWord* return_type = malloc(sizeof(SyntaxWord));
	return_type->token = blank_token();
	return_type->next = NULL;
	SyntaxWord* name = malloc(sizeof(SyntaxWord));
	name->token = copy_token(T->tokens);
	name->next = NULL;
	node->words = name;
	T->tokens = T->tokens->next->next;

	while (T->tokens && T->tokens->type != ')') {
		SyntaxWord* argument = malloc(sizeof(SyntaxWord));
		argument->token = blank_token();
		while (T->tokens && T->tokens->type != ',' && T->tokens->type != ')') {
			append_token_copy(argument->token, T->tokens);
			T->tokens = T->tokens->next;	
		}
		/* append to return_type because the order is name, return_type, args */
		append_word(return_type, argument);
		if (T->tokens->type == ')') {
			break;
		}
		T->tokens = T->tokens->next;
	}
	
	T->tokens = T->tokens->next->next;
	
	while (T->tokens->type != '{') {
		append_token_copy(return_type->token, T->tokens);
		T->tokens = T->tokens->next;
	}

	T->tokens = T->tokens->next;
	name->next = return_type;
	append_to_block(T, node);
	jump_in(T, node->block);

}

static void
parse_return(SyntaxTree* T) {
	/* begins on token RETURN */
	T->tokens = T->tokens->next;
	SyntaxNode* node = new_node(T, RETURN, 0);
	SyntaxWord* expression = malloc(sizeof(SyntaxWord));
	expression->token = parse_expression(T);
	expression->next = NULL;
	node->words = expression;
	append_to_block(T, node);
}

static void
parse_switch(SyntaxTree* T) {

}

static void
parse_case(SyntaxTree* T) {

}

static void
parse_continue(SyntaxTree* T) {
	/* begins on token CONTINUE */
	T->tokens = T->tokens->next->next;
	SyntaxNode* node = new_node(T, CONTINUE, 0);
	node->words = NULL;
	append_to_block(T, node);
}

static void
parse_break(SyntaxTree* T) {
	/* begins on token CONTINUE */
	T->tokens = T->tokens->next->next;
	SyntaxNode* node = new_node(T, BREAK, 0);
	node->words = NULL;
	append_to_block(T, node);
}

static void
parse_for(SyntaxTree* T) {
	/* begins on token FOR */
	T->tokens = T->tokens->next;
	SyntaxNode* node = new_node(T, FOR, 1);
	SyntaxWord* init, *condition, *inc;
	init = malloc(sizeof(SyntaxWord));
	condition = malloc(sizeof(SyntaxWord));
	inc = malloc(sizeof(SyntaxWord));
	init->token = parse_expression(T);
	init->next = condition;
	condition->token = parse_expression(T);
	condition->next = inc;
	inc->token = parse_expression_count(T, -1, '{');
	inc->next = NULL;
	node->words = init;
	append_to_block(T, node);
	jump_in(T, node->block);
}
