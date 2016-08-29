#ifndef PARSE_H
#define PARSE_H

#include "lex.h"

#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"

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

extern unsigned int op_pres[256];

TreeBlock* generate_tree(Token*);
void print_block(TreeBlock*, unsigned int);

#endif
