#ifndef PARSE_H
#define PARSE_H

#include "lex.h"

typedef struct Tree Tree;
typedef struct TreeNode TreeNode;
typedef struct TreeWord TreeWord;
typedef struct TreeBlock TreeBlock;
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

struct TreeWord {
	Token* token;
	struct TreeWord* next;
};

struct TreeBlock {
	TreeNode* parent_node;
	TreeNode* children;
};

struct TreeNode {
	NodeType type;
	TreeWord* words;
	TreeNode* next;
	TreeNode* prev;
	TreeBlock* block;
	TreeBlock* parent_block;
};

struct Tree {
	TreeNode* nodes;
	TreeBlock* current_block;
	TreeBlock* root_block;
	Token* tokens;
};

TreeBlock* generate_tree(Token*);
void print_block(TreeBlock*, unsigned int);

#endif
