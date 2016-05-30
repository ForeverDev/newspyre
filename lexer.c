#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

Token*
Lexer_convertToTokens(const char* source) {
	Lexer L;
	L.tokens = NULL;
	L.line = 1;

	char c;
	while ((c = *source++)) {
		if (c == '\n') {
			L.line++;
		} else if (c == ' ' || c == '\t') {
			continue;
		} else if (c == ';') {
			while (*source && *source != '\n') source++;
		} else if (c == '\'') {
			char* word = (char *)malloc(128);
			if (*source == '\\') {
				char result = 0;
				switch (*++source) {
					case '\\': result = '\\'; break;
					case 't':  result = '\t'; break;
					case 'n':  result = '\n'; break;
				}
				sprintf(word, "%d", result);
			} else {
				sprintf(word, "%d", *source++);
			}
			source++;
			Lexer_appendToken(&L, word, NUMBER);
		} else if (c == '"') {
			char* word;
			size_t len = 0;
			const char* at = source;
			while (*at++ != '"') len++;
			at--;
			word = (char *)malloc(len + 1);
			for (int i = 0; i < len; i++) {
				switch (*source) {
					case '\\':
						switch (*++source) {
							case 'n':
								word[i] = '\n';
								break;
							case 't':
								word[i] = '\t';
								break;
						}
						break;
					default:
						if (*source == '"') break;
						word[i] = *source;
						break;
				}
				source++;
			}
			word[len] = 0;
			source++;
			Lexer_appendToken(&L, word, LITERAL);
		} else if (ispunct(c) && c != '_') {
			char word[2];
			word[0] = c;
			word[1] = 0;
			Lexer_appendToken(&L, word, PUNCT);
		} else if (isdigit(c)) {
			//if (*source == 'x' && c == '0') source += 2;
			char* word;
			size_t len = 0;
			const char* at = source;
			while (isalnum(*at++) && ++len);
			at--;
			word = (char *)malloc(len + 2);	
			memcpy(word, source - 1, len + 1);
			word[len + 1] = 0;
			source += len;
			Lexer_appendToken(&L, word, NUMBER);
		} else if (isalpha(c) || c == '_') {
			char* word;
			size_t len = 0;
			const char* at = source;
			while ((isalnum(*at) || *at == '_') && ++len) at++;
			word = (char *)malloc(len + 2);	
			memcpy(word, source - 1, len + 1);
			word[len + 1] = 0;
			source += len;
			Lexer_appendToken(&L, word, IDENTIFIER);
		}
	}	

	/* tokens on heap, L on stack */
	return L.tokens;
}

static void
Lexer_appendToken(Lexer* L, const char* word, TokenType type) {
	Token* token = (Token *)malloc(sizeof(Token));
	token->next = NULL;
	token->prev = NULL;
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
		token->prev = at;
	}
}

static void
Lexer_printTokens(Lexer* L) {
	Token* at = L->tokens;
	while (at) {
		printf("%s (%s)\n", at->word, (
			at->type == PUNCT ? "operator" :
			at->type == NUMBER ? "number" : 
			at->type == IDENTIFIER ? "identifier" :
			at->type == LITERAL ? "literal" : "?"
		));		
		at = at->next;
	}
}
