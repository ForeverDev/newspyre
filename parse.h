#ifndef PARSE_H
#define PARSE_H

#include <stdint.h>
#include "lex.h"

typedef struct Tree Tree;
typedef struct TreeNode TreeNode;
typedef struct TreeWord TreeWord;
typedef struct TreeBlock TreeBlock;
typedef struct TreeVariable TreeVariable;
typedef enum NodeType NodeType;

/* instead of a nasty linked list of modifiers, just use masks */
#define MOD_CONST (0x1 << 0)
#define MOD_STATIC (0x1 << 1)
#define MOD_UNSIGNED (0x1 << 2)
#define MOD_SIGNED (0x1 << 3)

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
	ASSIGNMENT,
	ROOT
};

struct TreeVariable {
	char* identifier;
	char* datatype;
	uint16_t modifiers;
	unsigned int size; /* in bytes */
	unsigned int offset;
	int is_arg;
	TreeVariable* next;
};

struct TreeWord {
	Token* token;
	struct TreeWord* next;
};

struct TreeBlock {
	TreeNode* parent_node;
	TreeNode* children;
	TreeVariable* locals;
};

struct TreeNode {
	unsigned int line;
	NodeType type;
	TreeWord* words;
	TreeNode* next;
	TreeNode* prev;
	TreeBlock* block;
	TreeBlock* parent_block;

	TreeVariable* variable; /* only applicable if DECLARATION or FUNCTION */
	TreeVariable* ret; /* return type of FUNCTION */
	unsigned int nargs; /* only applicable if FUNCTION */
};

struct Tree {
	TreeNode* nodes;
	TreeBlock* current_block;
	TreeBlock* root_block;
	Token* tokens;
	unsigned int current_offset;
};

TreeBlock* generate_tree(Token*);

void print_block(TreeBlock*, unsigned int);

#endif
