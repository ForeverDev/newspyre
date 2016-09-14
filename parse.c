#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "parse.h"

static void parse_if(Tree*);
static void parse_else(Tree*);
static void parse_while(Tree*);
static void parse_do_while(Tree*);
static void parse_function(Tree*);
static void parse_return(Tree*);
static void parse_switch(Tree*);
static void parse_case(Tree*);
static void parse_continue(Tree*);
static void parse_break(Tree*);
static void parse_for(Tree*);
static void parse_declaration(Tree*);
static void parse_error(Tree*, const char*, ...);
static void parse_statement(Tree*);
static void jump_out(Tree*);
static void jump_in(Tree*, TreeBlock*);

static TreeVariable* parse_variable(Tree*);
static inline void append_token_copy(Token*, Token*);
static inline Token* parse_expression(Tree*);
static Token* parse_expression_count(Tree*, unsigned int, unsigned int);
static TreeNode* get_node_tail(Tree*);
static Token* copy_token(Token*);
static void append_to_block(Tree*, TreeNode*);
static void append_node(TreeNode*, TreeNode*);
static TreeNode* new_node(Tree*, unsigned int, int);
static void append_word(TreeWord*, TreeWord*);
static void append_var(TreeVariable*, TreeVariable*);
static void fix_connections(TreeNode*);
static void parse_datatype(Tree*, TreeVariable*);

static void
parse_datatype(Tree* T, TreeVariable* var) {
	while (T->tokens 
		&& T->tokens->type != ',' 
		&& T->tokens->type != ')'
		&& T->tokens->type != '{'
	) {
		if (!strcmp(T->tokens->word, "const")) {
			var->modifiers |= MOD_CONST;
		} else if (!strcmp(T->tokens->word, "static")) {
			var->modifiers |= MOD_STATIC;
		} else if (!strcmp(T->tokens->word, "unsigned")) {
			var->modifiers |= MOD_UNSIGNED;
		} else {
			var->datatype = T->tokens->word;
		}
		T->tokens = T->tokens->next;	
	}
}

static TreeVariable*
parse_variable(Tree* T) {
	TreeVariable* local = malloc(sizeof(TreeVariable));
	local->identifier = T->tokens->word; 
	local->next = NULL;
	local->modifiers = 0;
	local->size = 8;
	local->offset = 0;
	local->datatype = NULL;
	T->tokens = T->tokens->next->next;
	parse_datatype(T, local);
	return local;
}

static void 
append_word(TreeWord* head, TreeWord* word) {
	TreeWord* at = head;
	while (at->next) {
		at = at->next;
	}
	at->next = word;
	word->next = NULL;
}

static void
append_var(TreeVariable* head, TreeVariable* var) {
	TreeVariable* at = head;
	while (at->next) {
		at = at->next;
	}
	at->next = var;
	var->next = NULL;
}

static TreeNode*
new_node(Tree* T, unsigned int type, int has_block) {
	TreeNode* node = malloc(sizeof(TreeNode));
	node->type = type;
	node->words = malloc(sizeof(TreeWord));
	node->words->token = NULL;
	node->words->next = NULL;
	node->next = NULL;
	node->prev = NULL;
	node->block = NULL;
	if (has_block) {
		node->block = malloc(sizeof(TreeBlock));
		node->block->parent_node = node;
		node->block->children = NULL;
		node->block->locals = NULL;
	}
	node->parent_block = T->current_block;
	return node;
}

static void
parse_error(Tree* T, const char* format, ...) {
	va_list list;
	va_start(list, format);

	vprintf(format, list);
	printf("\nline: %d\n", T->tokens->line);
	va_end(list);

	exit(1);
}

static void
jump_out(Tree* T) {
	if (!T->current_block->parent_node) {
		printf("done with program\n");
		exit(0);	
	}
	T->current_block = T->current_block->parent_node->parent_block;
	T->tokens = T->tokens->next;
}

static void
jump_in(Tree* T, TreeBlock* block) {
	T->current_block = block;
}

static void
append_node(TreeNode* head, TreeNode* node) {
	TreeNode* at = head;
	if (at->type == 0) {
		at->type = node->type;
		at->words = node->words;
		at->next = NULL;
		at->prev = NULL;
		at->block = node->block;
		at->parent_block = node->parent_block;
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
append_to_block(Tree* T, TreeNode* node) {
	if (!T->current_block->children) {
		T->current_block->children = calloc(1, sizeof(TreeNode));
	}
	if (node->type == DECLARATION) {
		TreeVariable* local = malloc(sizeof(TreeVariable));
		local->identifier = node->words->token->word;
		local->next = NULL;
		local->modifiers = 0;
		local->offset = 0;
		/* scan for variable modifiers */
		Token* modifier = node->words->next->token;
		while (modifier->next) {
			if (!strcmp(modifier->word, "const")) {
				local->modifiers |= MOD_CONST; 
			} else if (!strcmp(modifier->word, "static")) {
				local->modifiers |= MOD_STATIC;
			} else if (!strcmp(modifier->word, "unsigned")) {
				local->modifiers |= MOD_UNSIGNED;
			} else if (!strcmp(modifier->word, "signed")) {
				local->modifiers |= MOD_SIGNED;
			} else {
				parse_error(T, "invalid variable modifier \"%s\"", modifier->word);
			}
			modifier = modifier->next;
		}
		/* detect the datatype of the variable */
		local->datatype = modifier->word;
		if (!strcmp(local->datatype, "real") || !strcmp(local->datatype, "float") || !strcmp(local->datatype, "string")) {
			local->size = 1;
		} else {
			local->size = 0;
		}
		/* append to variable to the list of locals */
		unsigned int offset = 0;
		if (T->current_block->locals) {
			TreeVariable* at = T->current_block->locals;
			while (at) {
				if (!strcmp(at->identifier, local->identifier)) {
					parse_error(T, "duplicate variable %s", local->identifier);	
				}
				offset++;
				if (!at->next) break;
				at = at->next;
			}
			local->offset = offset;
			at->next = local;
		} else {
			local->offset = 0;
			T->current_block->locals = local;
		}
	}
	node->parent_block = T->current_block;
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

static TreeNode*
get_node_tail(Tree* T) {
	TreeNode* at = T->nodes;
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
parse_expression(Tree* T) {
	return parse_expression_count(T, 0, ';');
}

static Token*
parse_expression_count(Tree* T, unsigned int inc, unsigned int dec) {
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


	return expression;
}

#define INDENT() for (int i = 0; i < indent; i++) printf("\t")

void 
print_block(TreeBlock* block, unsigned int indent) {
	TreeNode* node = block->children;
	if (!node) {
		return;
	}
	TreeWord* word;
	while (node) {
		word = node->words;
		INDENT();
		printf("(%s", (
			node->type == STATEMENT ? "STATEMENT" :
			node->type == DECLARATION ? "DECLARATION" :
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
			node->type == ASSIGNMENT ? "ASSIGNMENT" :
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

static void
parse_statement(Tree* T) {
	Token* expression = parse_expression(T);
	for (Token* i = expression; i; i = i->next) {
		if (i->type == TYPE_ASSIGN) {
			TreeNode* node = new_node(T, ASSIGNMENT, 0);
			/* i is currently pointing at the equals...
			 * detatch the LHS from the RHS and assign to
			 * node->word accordingly
			 */
			node->words->token = expression;
			TreeWord* rhs = malloc(sizeof(TreeWord));
			rhs->next = NULL;
			rhs->token = i->next;	
			node->words->next = rhs;
			i->next->prev = NULL;
			i->next = NULL;
			i->prev->next = NULL;
			append_to_block(T, node);
			/* TODO fix memory leak @i */
			return;					
		}
	}
	TreeNode* node = new_node(T, STATEMENT, 0);
	node->words->token = expression;
	append_to_block(T, node);
}

static void
parse_if(Tree* T) {
	/* begins on token IF */
	T->tokens = T->tokens->next;
	/* now on first token of condition */
	if (T->tokens->type == '{') {
		parse_error(T, "Expected condition in if statement");	
	}
	TreeNode* node = new_node(T, IF, 1); 
	node->words->token = parse_expression_count(T, 0, '{');
	append_to_block(T, node);
	jump_in(T, node->block);
}

static void
parse_else(Tree* T) {
	/* begins on token ELSE */
	T->tokens = T->tokens->next;
	if (T->tokens->type != '{') {
		parse_error(T, "Expected '{' after else");
	}
	TreeNode* node = new_node(T, ELSE, 1);
	append_to_block(T, node);
	jump_in(T, node->block);
}

static void
parse_while(Tree* T) {
	/* begins on token WHILE */
	T->tokens = T->tokens->next;
	/* now on first token of condition */
	if (T->tokens->type == '{') {
		parse_error(T, "Expected condition in while loop");	
	}
	TreeNode* node = new_node(T, WHILE, 1); 
	node->words->token = parse_expression_count(T, 0, '{');
	append_to_block(T, node);
	jump_in(T, node->block);
}

static void
parse_do_while(Tree* T) {

}

/* syntax function name(arg_name : arg_type, arg_name : arg_type) -> return_type { ... } */
/* WORDS
	0: function name
	1: function return type
	2: arg0 name
	3: arg0 return type
	...
*/

/* TODO MAKE A BETTER STORAGE METHOD FOR FUNCTION PARAMETERS USING TreeVariable */
static void
parse_function(Tree* T) {
	/* begins on token FUNCTION */
	T->tokens = T->tokens->next;
	/* now on name of function */
	TreeNode* node = new_node(T, FUNCTION, 1);
	TreeVariable* return_var_tmp = malloc(sizeof(TreeVariable));
	return_var_tmp->next = NULL;
	TreeWord* name = malloc(sizeof(TreeWord));
	name->token = copy_token(T->tokens);
	name->next = NULL;
	node->words = name;
	node->nargs = 0;
	T->tokens = T->tokens->next->next;

	while (T->tokens && T->tokens->type != ')') {
		node->nargs++;
		TreeVariable* local = parse_variable(T);
		append_var(return_var_tmp, local);
		if (T->tokens->type == ')') {
			break;
		}
		T->tokens = T->tokens->next;
	}
	
	T->tokens = T->tokens->next->next;
	TreeVariable* return_var = malloc(sizeof(TreeVariable));
	parse_datatype(T, return_var);
	return_var->identifier = NULL;
	return_var->next = return_var_tmp->next;
	node->variable = return_var;
	printf("DEFINE %p (%s)\n", node, node->words->token->word);
	T->tokens = T->tokens->next;
	append_to_block(T, node);
	jump_in(T, node->block);
}

static void
parse_return(Tree* T) {
	/* begins on token RETURN */
	T->tokens = T->tokens->next;
	TreeNode* node = new_node(T, RETURN, 0);
	TreeWord* expression = malloc(sizeof(TreeWord));
	expression->token = parse_expression(T);
	expression->next = NULL;
	node->words = expression;
	append_to_block(T, node);
}

static void
parse_switch(Tree* T) {

}

static void
parse_case(Tree* T) {

}

static void
parse_continue(Tree* T) {
	/* begins on token CONTINUE */
	T->tokens = T->tokens->next->next;
	TreeNode* node = new_node(T, CONTINUE, 0);
	node->words = NULL;
	append_to_block(T, node);
}

static void
parse_break(Tree* T) {
	/* begins on token CONTINUE */
	T->tokens = T->tokens->next->next;
	TreeNode* node = new_node(T, BREAK, 0);
	node->words = NULL;
	append_to_block(T, node);
}

static void
parse_for(Tree* T) {
	/* begins on token FOR */
	T->tokens = T->tokens->next;
	TreeNode* node = new_node(T, FOR, 1);
	TreeWord* init, *condition, *inc;
	init = malloc(sizeof(TreeWord));
	condition = malloc(sizeof(TreeWord));
	inc = malloc(sizeof(TreeWord));
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

/* starts on the identifier, syntax is   x : datatype; */
static void
parse_declaration(Tree* T) {
	TreeNode* node = new_node(T, DECLARATION, 0);
	TreeWord* identifier = malloc(sizeof(TreeWord));
	TreeWord* datatype = malloc(sizeof(TreeWord));
	identifier->token = copy_token(T->tokens);
	identifier->next = datatype;
	T->tokens = T->tokens->next->next;
	datatype->token = parse_expression(T);
	datatype->next = NULL;
	node->words = identifier;
	append_to_block(T, node); 
}

static void
fix_connections(TreeNode* node) {
	TreeNode* at = node;
	while (at) {
		if (at->block) {
			TreeNode* child = at->block->children;
			while (child) {
				child->parent_block->parent_node = at;
				fix_connections(child);
				child = child->next;
			}
		}
		at = at->next;
	}
}

TreeBlock*
generate_tree(Token* tokens) {
	Tree* T = malloc(sizeof(Tree));
	T->tokens = tokens;
	T->nodes = malloc(sizeof(TreeNode));
	T->nodes->type = ROOT;
	T->nodes->next = NULL;
	T->nodes->prev = NULL;
	T->nodes->block = malloc(sizeof(TreeBlock));
	T->nodes->block->parent_node = T->nodes;
	T->nodes->block->children = NULL;
	T->nodes->parent_block = NULL;
	T->current_block = T->nodes->block;
	T->root_block = T->current_block;
	
	while (T->tokens) {
		switch (T->tokens->type) {
			case TYPE_IF: parse_if(T); break;
			case TYPE_ELSE: parse_else(T); break;
			case TYPE_WHILE: parse_while(T); break;
			case TYPE_DO: parse_do_while(T); break;
			case TYPE_FUNCTION: parse_function(T); break;
			case TYPE_RETURN: parse_return(T); break;
			case TYPE_SWITCH: parse_switch(T); break;
			case TYPE_CASE: parse_case(T); break;
			case TYPE_CONTINUE: parse_continue(T); break;
			case TYPE_BREAK: parse_break(T); break;
			case TYPE_FOR: parse_for(T); break;
			case TYPE_IDENTIFIER: 
				if (T->tokens->next && T->tokens->next->type == TYPE_COLON) {
					parse_declaration(T);
				} else {
					parse_statement(T);
				}
				break;
			case '}': jump_out(T); break;
			default: parse_statement(T); break;
		}
	}

	/* TODO remove need for this... still need to figure
	   out what the fuck is causing the list pointer problems
	*/
	fix_connections(T->nodes);

	TreeBlock* block = T->root_block;

	/* TODO cleanup correctly */
	free(T);

	return block;
}

