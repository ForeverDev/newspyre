#ifndef ASSEMBLER_LEX_H
#define ASSEMBLER_LEX_H

typedef struct AssemblerToken AssemblerToken;
typedef struct Lexer Lexer;
typedef enum AssemblerTokenType AssemblerTokenType;

enum AssemblerTokenType {
	NOTOK, PUNCT, NUMBER, IDENTIFIER, LITERAL
};

struct AssemblerToken {
	char*					word;
	unsigned int			line;
	AssemblerTokenType		type;
	AssemblerToken*			next;
	AssemblerToken*			prev;
};

struct Lexer {
	AssemblerToken*	tokens;	
	unsigned int	line;
};

AssemblerToken*		Lexer_convertToAssemblerTokens(const char*);
static void			Lexer_appendAssemblerToken(Lexer*, const char*, AssemblerTokenType);
static void			Lexer_printAssemblerTokens(Lexer*);

#endif
