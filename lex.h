#ifndef LEX_H
#define LEX_H

typedef struct Lexer Lexer;
typedef struct Token Token;
typedef enum TokenType TokenType;

#define TYPE_IF			1
#define TYPE_ELSE		2
#define TYPE_WHILE		3
#define TYPE_DO			4
#define TYPE_FUNCTION	5
#define TYPE_RETURN		6
#define TYPE_SWITCH		7
#define TYPE_CASE		8
#define TYPE_CONTINUE	9
#define TYPE_BREAK		10
#define TYPE_FOR		11
#define TYPE_IDENTIFIER	12
#define TYPE_NUMBER		13
#define TYPE_STRING		14
#define TYPE_LOGAND		128
#define TYPE_LOGOR		129
#define TYPE_SHR		130
#define TYPE_SHL		131
#define TYPE_INC		132
#define TYPE_INCBY		133
#define TYPE_DEC		134
#define TYPE_DECBY		135
#define TYPE_MULBY		136
#define TYPE_DIVBY		137
#define TYPE_MODBY		138
#define TYPE_ANDBY		139
#define TYPE_ORBY		140
#define	TYPE_XORBY		141
#define TYPE_SHRBY		142
#define TYPE_SHLBY		143
#define TYPE_ARROWBY	144
#define TYPE_EQ			145
#define TYPE_NOTEQ		146
#define TYPE_GE			147
#define TYPE_LE			148
#define TYPE_ARROW		149

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
		->			149
	*/
	unsigned int	type;

	Token*			next;
};

Token* generate_tokens(const char*);
void append_token(Token*, char*, unsigned int, unsigned int);
void print_tokens(Token*);
Token* blank_token();

#endif
