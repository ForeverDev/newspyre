#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "parse.h"

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
static void handle_expression(SyntaxTree*);
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
	Token* new = malloc(sizeof(token));
	memcpy(new, token, sizeof(token));
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
	return expression;
}

#define INDENT() for (int i = 0; i < indent; i++) printf("\t")

static void print_block(SyntaxBlock* block, unsigned int indent) {
	SyntaxNode* node = block->children;
	if (!node) {
		return;
	}
	SyntaxWord* word;
	while (node) {
		word = node->words;
		INDENT();
		printf("(%s\n", (
			node->type == TYPE_IF ? "IF" :
			node->type == TYPE_ELSE ? "ELSE" :
			node->type == TYPE_WHILE ? "WHILE" :
			node->type == TYPE_DO ? "DO_WHILE" :
			node->type == TYPE_FUNCTION ? "FUNCTION" :
			node->type == TYPE_RETURN ? "RETURN" :
			node->type == TYPE_SWITCH ? "SWITCH" :
			node->type == TYPE_CASE ? "CASE" :
			node->type == TYPE_CONTINUE ? "CONTINUE" :
			node->type == TYPE_BREAK ? "BREAK" :
			node->type == TYPE_FOR ? "FOR" :
			node->type == TYPE_IDENTIFIER ? "IDENTIFIER" :
			node->type == TYPE_NUMBER ? "NUMBER" :
			node->type == TYPE_STRING ? "STRING" : "?????"
		));
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
		printf(")");
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
			default: handle_expression(T); break; 
		}
	}
	
	print_tree(T);

	return T;
}

static void
handle_expression(SyntaxTree* T) {

}

static void
parse_if(SyntaxTree* T) {
	/* begins on token IF */
	T->tokens = T->tokens->next;
	/* now on first token of condition */
	if (T->tokens->type == '{') {
		parse_error(T, "Expected condition in if statement");	
	}
	SyntaxNode* node = malloc(sizeof(SyntaxNode));
	node->type = IF;
	node->next = NULL;
	node->prev = NULL;
	node->block = malloc(sizeof(SyntaxBlock));
	node->block->children = NULL;
	node->block->node_parent = node;
	node->block_parent = T->current_block;
	node->words = malloc(sizeof(SyntaxWord));
	node->words->token = parse_expression_count(T, 0, '{');
	node->words->next = NULL;
	append_to_block(T, node);
	jump_in(T, node->block);
}

static void
parse_else(SyntaxTree* T) {

}

static void
parse_while(SyntaxTree* T) {

}

static void
parse_do_while(SyntaxTree* T) {

}

static void
parse_function(SyntaxTree* T) {

}

static void
parse_return(SyntaxTree* T) {

}

static void
parse_switch(SyntaxTree* T) {

}

static void
parse_case(SyntaxTree* T) {

}

static void
parse_continue(SyntaxTree* T) {

}

static void
parse_break(SyntaxTree* T) {

}

static void
parse_for(SyntaxTree* T) {

}
