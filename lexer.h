#ifndef LEXER_H
#define LEXER_H

typedef struct Token Token;
typedef struct Lexer Lexer;
typedef enum TokenType TokenType;

enum TokenType {
	NOTOK, PUNCT, NUMBER, IDENTIFIER	
};

struct Token {
	char*			word;
	unsigned int	line;
	TokenType		type;
	Token*			next;
};

struct Lexer {
	Token*			tokens;	
	unsigned int	line;
};

Token*				Lexer_convertToTokens(const char*);
static void			Lexer_appendToken(Lexer*, const char*, TokenType);
static void			Lexer_printTokens(Lexer*);

#endif
