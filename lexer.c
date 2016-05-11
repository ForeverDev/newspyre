#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

Token*
Lexer_convertToTokens(const char* source) {
	Lexer L;
	L.tokens = NULL;
	L.line = 0;

	char c;
	while ((c = *source++)) {
		if (c == ' ' || c == '\t') {
			continue;
		} else if (ispunct(c)) {
			char word[2];
			word[0] = c;
			word[1] = 0;
			Lexer_appendToken(&L, word, PUNCT);
		} else if (isnumber(c)) {
			char* word;
			size_t len = 0;
			const char* at = source;
			while (isnumber(*at++) && ++len);
			at--;
			word = (char *)malloc(len + 1);	
			memcpy(word, source - 1, len + 1);
			source += len;
			Lexer_appendToken(&L, word, NUMBER);
		} else if (isalpha(c)) {
			char* word;
			size_t len = 0;
			const char* at = source;
			while (isalnum(*at++) && ++len);
			word = (char *)malloc(len + 1);	
			memcpy(word, source - 1, len + 1);
			source += len;
			Lexer_appendToken(&L, word, IDENTIFIER);
		}
	}	
	
	Lexer_printTokens(&L);
	
	/* tokens on heap, L on stack */
	return L.tokens;
}

static void
Lexer_appendToken(Lexer* L, const char* word, TokenType type) {
	Token* token = (Token *)malloc(sizeof(Token));
	token->next = NULL;
	token->line = L->line;
	token->type = type;
	size_t length = strlen(word);
	token->word = (char *)malloc(length + 1);
	strcpy(token->word, word);
	token->word[length] = 0;
	if (!L->tokens) {
		L->tokens = token;
	} else {
		Token* at = L->tokens;
		while (at->next) at = at->next;
		at->next = token;
	}
}

static void
Lexer_printTokens(Lexer* L) {
	Token* at = L->tokens;
	while (at) {
		printf("%s (%s)\n", at->word, (
			at->type == PUNCT ? "operator" :
			at->type == NUMBER ? "number" : 
			at->type == IDENTIFIER ? "identifier" : "?"
		));		
		at = at->next;
	}
}
