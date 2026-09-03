#ifndef LEXER_H
#define LEXER_H
#include <stdbool.h>

/*
SonjC Lexer Header File
Written Aug 2026
Defines token types and other constant values for the lexer.
*/

typedef enum {
    TOKEN_NONE,
    TOKEN_KEYWORD,
    TOKEN_IDENTIFIER,
    TOKEN_PUNCTUATION,
    TOKEN_STRING,
    TOKEN_INTEGER,
    TOKEN_FLOAT,
    TOKEN_FIXED,
    TOKEN_CHAR,
    TOKEN_OPERATOR,
    TOKEN_DIRECTIVE,
    TOKEN_EOF
} TokenType;

typedef struct {
    TokenType type;
    char *text;
    int length;
} Token;

typedef struct {
    char *source;
    int pos;
    int line;
    int col;

    bool stringMode;
    bool charMode;
    bool charFilled;
    char currentText[300];
} Lexer;

char *tokentypeToString(TokenType type);
Token nextToken(Lexer *lexer); 

#endif