#ifndef LEX_H
#define LEX_H

typedef struct Lexer Lexer;
typedef struct Token Token;
typedef enum TokenType TokenType;

struct Token {
	char*			word;
	unsigned int	line;	
	/* 
	TYPES:
		IF:			1
		ELSE:		2
		WHILE:		3
		DO:			4
		FUNCTION:	5
		RETURN:		6
		SWITCH:		7
		CASE:		8
		CONTINUE:	9
		BREAK:		10
		FOR:		11
		IDENTIFIER:	12
		NUMBER:		13
		STRING:		14

		&&			128
		||			129
		>>			130
		<<			131
		++			132
		+=			133
		--			134
		-=			135
		*=			136
		/=			137
		%=			138
		&=			139
		|=			140
		^=			141
		>>=			142
		<<=			143
		->=			144
		==			145
		!=			146		
		>=			147
		<=			148
	*/
	unsigned int	type;

	Token*			next;
};

Token* generate_tokens(const char*);

#endif
