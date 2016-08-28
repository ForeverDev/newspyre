#ifndef PARSE_H
#define PARSE_H

#include "lex.h"

typedef struct SyntaxTree SyntaxTree;
typedef struct SyntaxNode SyntaxNode;
typedef struct SyntaxWord SyntaxWord;
typedef struct SyntaxBlock SyntaxBlock;
typedef enum NodeType NodeType;

enum NodeType {
	NOTYPE,
	STATEMENT,
	IF,
	ELSE,
	WHILE,
	DO_WHILE,
	FOR,
	SWITCH,
	CASE,
	RETURN,
	CONTINUE,
	BREAK,
	FUNCTION,
	DECLARATION,
	ROOT
};

struct SyntaxWord {
	Token* token;
	struct SyntaxWord* next;
};

struct SyntaxBlock {
	SyntaxNode* node_parent;
	SyntaxNode* children;
};

struct SyntaxNode {
	NodeType type;
	SyntaxWord* words;
	SyntaxNode* next;
	SyntaxNode* prev;
	SyntaxBlock* block;
	SyntaxBlock* block_parent;
};

struct SyntaxTree {
	SyntaxNode* nodes;
	SyntaxBlock* current_block;
	SyntaxBlock* root_block;
	Token* tokens;
};

extern unsigned int op_pres[256];

SyntaxTree* generate_tree(Token*);
void print_tree(SyntaxTree*);

#endif
