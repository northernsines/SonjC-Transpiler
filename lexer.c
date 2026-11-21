#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
SonjC Lexer
Written Aug 2026
Converts C source code into tokens to be used by the SonjC transpiler.
*/

void advance(Lexer* lexer) {
    lexer->col++;
    lexer->pos++;
}

void retreat(Lexer* lexer) {
    lexer->col--;
    lexer->pos--;
}

void newLine(Lexer* lexer) {
    lexer->col = 0;
    lexer->line++;
}

Token returnEOF(Lexer* lexer) {
    *lexer->current_text = '\0';
    return (Token){
        .length = 1,
        .text = lexer->current_text,
        .type = TOKEN_EOF};
}

bool isNumber(char currentChar) {
    return ('0' <= currentChar && currentChar <= '9');
}

//STUB add all keywords
bool isKeyword(char* text) {
    return
        // standard C types/qualifiers
        !strcmp(text, "int") ||
        !strcmp(text, "float") ||
        !strcmp(text, "char") ||
        !strcmp(text, "bool") ||
        !strcmp(text, "void") ||
        !strcmp(text, "const") ||
        !strcmp(text, "volatile") ||
        !strcmp(text, "raw") ||
        !strcmp(text, "unsigned") ||
        !strcmp(text, "long") ||
        !strcmp(text, "short") ||
        !strcmp(text, "static") ||
        !strcmp(text, "extern") ||
        !strcmp(text, "auto") ||
        !strcmp(text, "register") ||
        !strcmp(text, "inline") ||
        !strcmp(text, "true") ||
        !strcmp(text, "false") ||
        // standard C control flow
        !strcmp(text, "return") ||
        !strcmp(text, "if") ||
        !strcmp(text, "else") ||
        !strcmp(text, "for") ||
        !strcmp(text, "while") ||
        !strcmp(text, "do") ||
        !strcmp(text, "continue") ||
        !strcmp(text, "break") ||
        !strcmp(text, "switch") ||
        !strcmp(text, "case") ||
        !strcmp(text, "default") ||
        !strcmp(text, "goto") ||
        !strcmp(text, "label") ||
        // standard C type/struct related
        !strcmp(text, "enum") ||
        !strcmp(text, "union") ||
        !strcmp(text, "typedef") ||
        !strcmp(text, "sizeof") ||
        // GNU/common extension
        !strcmp(text, "typeof") ||
        !strcmp(text, "asm") ||
        // SonjC-specific additions
        !strcmp(text, "elif") ||
        !strcmp(text, "fixed") ||
        !strcmp(text, "iterate") ||
        !strcmp(text, "when") ||
        !strcmp(text, "repeat") ||
        !strcmp(text, "until") ||
        !strcmp(text, "forever") ||
        !strcmp(text, "object") ||
        !strcmp(text, "import") ||
        !strcmp(text, "namespace") ||
        !strcmp(text, "public") ||
        !strcmp(text, "private");
}

//STUB add all operators
bool isOperator(char currentChar) {
    return currentChar == '+' ||
           currentChar == '-' ||
           currentChar == '=' ||
           currentChar == '*' ||
           currentChar == '/' ||
           currentChar == '%' ||
           currentChar == '&' ||
           currentChar == '|' ||
           currentChar == '^' ||
           currentChar == '!' ||
           currentChar == '~' ||
           currentChar == '<' ||
           currentChar == '>' ||
           currentChar == '?' ||
           currentChar == '.';
}

//STUB add all multichar operators
bool isMulticharOperator(char* text) {
    return
        // three-char
        !strcmp(text, "<<=") ||
        !strcmp(text, ">>=") ||
        !strcmp(text, "...") ||
        // two-char
        !strcmp(text, "&&") ||
        !strcmp(text, "&=") ||
        !strcmp(text, "%=") ||
        !strcmp(text, "!=") ||
        !strcmp(text, "*=") ||
        !strcmp(text, "++") ||
        !strcmp(text, "+=") ||
        !strcmp(text, "--") ||
        !strcmp(text, "-=") ||
        !strcmp(text, "->") ||
        !strcmp(text, "/=") ||
        !strcmp(text, "<=") ||
        !strcmp(text, "<<") ||
        !strcmp(text, "==") ||
        !strcmp(text, ">=") ||
        !strcmp(text, ">>") ||
        !strcmp(text, "^=") ||
        !strcmp(text, "||") ||
        !strcmp(text, "|=");
}

//STUB add all list of punctuation
bool isPunctuation(char currentChar) {
    return currentChar == '{' ||
           currentChar == '}' ||
           currentChar == '(' ||
           currentChar == ')' ||
           currentChar == '[' ||
           currentChar == ']' ||
           currentChar == ':' ||
           currentChar == ';' ||
           currentChar == ' ' ||
           currentChar == '\n' ||
           currentChar == ',';
}

void failIfOver(int size, Lexer* lexer) {
    if (size >= 299) {
        fprintf(stderr, "too long of identifier!, line: %d, col %d\n", lexer->line, lexer->col);
        exit(EXIT_FAILURE);
    } else
        return;
}

void skipNewline(Lexer* lexer) {
    newLine(lexer);
    advance(lexer);
}

int skipComment(Lexer* lexer) {
    char current_char = lexer->source[lexer->pos];

    if (current_char != '/')
        return 0;
    if (current_char == '\0')
        return 2;

    advance(lexer);
    current_char = lexer->source[lexer->pos];

    if (current_char == '/') {
        while (current_char != '\n') {
            if (current_char == '\0')
                return 2;
            current_char = lexer->source[lexer->pos];
        }

        newLine(lexer);
        return 1;
    } else if (current_char == '*') {
        while (1) {
            current_char = lexer->source[lexer->pos];

            if (current_char == '\0')
                return 2;

            if (current_char == '\n')
                newLine(lexer);

            if (current_char == '*' &&
                lexer->source[lexer->pos + 1] == '/') {
                advance(lexer); // consume '*'
                advance(lexer); // consume '/'
                return 1;
            }

            advance(lexer);
        }
    } else {
        retreat(lexer);
        return 0;
    }
}

Token handleCharLiteral(Lexer* lexer, int* currentSize) {
    char current_char = lexer->source[lexer->pos];

    if (lexer->char_mode == true && lexer->char_filled == true &&
        current_char != '\'' && current_char != '\\') //if filled 1 char (lexer->char_filled) but next char is not ' or \ and still in lexer->char_mode, error.
    {
        fprintf(stderr, "char literal takes too many args, line: %d, col %d\n",
                lexer->line, lexer->col);
        exit(EXIT_FAILURE);
    }

    if (lexer->char_mode == true && lexer->char_filled == false) //handle char itself
    {
        if (current_char == '\0')
            return returnEOF(lexer);
        if (current_char == '\\') //if \, keep as raw text, grab next char too
        {
            lexer->current_text[*currentSize] = current_char;
            (*currentSize)++;
            if (current_char == '\0')
                return returnEOF(lexer);
            advance(lexer);
            current_char = lexer->source[lexer->pos];
        }

        lexer->current_text[*currentSize] = current_char;
        (*currentSize)++;
        if (current_char == '\0')
            return returnEOF(lexer);
        lexer->char_filled = true;
        advance(lexer);

        return (Token){
            .length = *currentSize,
            .text = lexer->current_text,
            .type = TOKEN_CHAR};
    }

    return (Token){0};
}

bool handleQuotes(Lexer* lexer, TokenType* currentType) {
    char current_char = lexer->source[lexer->pos];
    *currentType = TOKEN_STRING;

    if (current_char == '\'') //handle single quotes themselves
    {
        if (lexer->char_mode == false)
            lexer->char_mode = true;
        else {
            lexer->char_mode = false;
            lexer->char_filled = false;
        }
        return true; // signals "a quote was consumed, caller should re-lex"
    }

    if (current_char == '"') //handle double quotes themselves
    {
        if (lexer->string_mode == false)
            lexer->string_mode = true;
        else
            lexer->string_mode = false;
        return true;
    }

    return false;
}

Token handleString(Lexer* lexer, int* currentSize) {
    char current_char = lexer->source[lexer->pos];

    if (lexer->string_mode) {
        while (current_char != '"') {
            if (current_char == '\0')
                return returnEOF(lexer);
            if (current_char == '\\') {
                lexer->current_text[*currentSize] = current_char;
                (*currentSize)++;
                advance(lexer);
                current_char = lexer->source[lexer->pos];
            }

            lexer->current_text[*currentSize] = current_char;
            (*currentSize)++;
            failIfOver(*currentSize, lexer);
            advance(lexer);
            current_char = lexer->source[lexer->pos];
        }

        lexer->current_text[*currentSize] = '\0';
        lexer->string_mode = false; // closing quote found, exit string mode

        return (Token){
            .length = *currentSize,
            .text = lexer->current_text,
            .type = TOKEN_STRING};
    }

    return (Token){0};
}

Token readNumber(Lexer* lexer, int* currentSize) {
    char current_char = lexer->source[lexer->pos];
    bool seen_dot = false;
    bool seen_f = false;

    while ((current_char >= '0' && current_char <= '9') || (current_char == '.' && seen_dot == false) || (current_char >= 'f' && seen_f == false)) //digit and single decimal is allowed, as well as f for fixed marker.
    {
        lexer->current_text[*currentSize] = current_char;
        (*currentSize)++;
        failIfOver(*currentSize, lexer);
        if (current_char == '.')
            seen_dot = true;
        if (current_char == 'f')
            seen_f = true;
        advance(lexer); //next pos
        current_char = lexer->source[lexer->pos];
    }

    TokenType number_type = TOKEN_INTEGER; //int by default
    if (seen_dot)
        number_type = TOKEN_FLOAT; //float by default if point detected
    if (seen_f)
        number_type = TOKEN_FIXED; //fixed if fixed specifier detected
    lexer->current_text[*currentSize] = '\0';

    return (Token){
        .length = *currentSize,
        .text = lexer->current_text,
        .type = number_type};
}

void readOperator(Lexer* lexer, int* currentSize) {
    char current_char = lexer->source[lexer->pos];

    advance(lexer); //next pos
    current_char = lexer->source[lexer->pos];

    if (isOperator(current_char)) {
        advance(lexer); //next pos
        lexer->current_text[1] = current_char;
        current_char = lexer->source[lexer->pos];
        (*currentSize)++;

        if (isOperator(current_char)) {
            lexer->current_text[2] = current_char;
            lexer->current_text[3] = '\0';

            if (!isMulticharOperator(lexer->current_text)) {
                lexer->current_text[2] = '\0';

                if (isMulticharOperator(lexer->current_text)) {
                    retreat(lexer);
                } else {
                    lexer->current_text[1] = '\0';
                    *currentSize = 1;
                    retreat(lexer);
                    retreat(lexer);
                }
            } else {
                (*currentSize)++;
            }
        } else {
            retreat(lexer);
            lexer->current_text[2] = '\0';

            if (!isMulticharOperator(lexer->current_text)) {
                lexer->current_text[1] = '\0';
                (*currentSize)--;
                retreat(lexer);
            }
        }
    } else {
        retreat(lexer);
    }
}

Token handlePunctuationOperatorNumber(Lexer* lexer, int* currentSize, TokenType* currentType) {
    char current_char = lexer->source[lexer->pos];

    if (isPunctuation(current_char) ||
        isOperator(current_char) ||
        isNumber(current_char)) {
        if (isPunctuation(current_char))
            *currentType = TOKEN_PUNCTUATION;
        else if (isOperator(current_char)) {
            *currentType = TOKEN_OPERATOR;
        }
        if (current_char == '\n')
            newLine(lexer); //new line if newline

        lexer->current_text[0] = current_char;
        (*currentSize)++;

        if (isOperator(current_char)) //check multichar operator
        {
            readOperator(lexer, currentSize);
        }
        if (isNumber(current_char)) //load multichar Number
        {
            advance(lexer);
            return readNumber(lexer, currentSize);
        }

        lexer->current_text[*currentSize] = '\0'; //null terminate
        return (Token){
            .length = *currentSize,
            .text = lexer->current_text,
            .type = *currentType};
    }

    return (Token){0};
}

Token handleIdentifier(Lexer* lexer, int* currentSize, TokenType* currentType) {
    char current_char = lexer->source[lexer->pos];

    if (!isPunctuation(current_char) &&
        !isOperator(current_char) &&
        current_char != '\'' &&
        current_char != '"') {
        while (!isPunctuation(current_char) &&
               !isOperator(current_char) &&
               current_char != '\'' &&
               current_char != '"') {
            lexer->current_text[*currentSize] = current_char;
            (*currentSize)++;
            failIfOver(*currentSize, lexer);
            advance(lexer); //next pos
            current_char = lexer->source[lexer->pos];
        }

        lexer->current_text[*currentSize] = '\0'; //add null terminator so strmcp works
        retreat(lexer);                          //back 1 position to set up next char as ending punct

        if (isKeyword(lexer->current_text))
            *currentType = TOKEN_KEYWORD;
        else
            *currentType = TOKEN_IDENTIFIER;

        return (Token){
            .length = *currentSize,
            .text = lexer->current_text,
            .type = *currentType};
    }

    return (Token){0};
}

Token nextToken(Lexer* lexer) {
    int current_size = 0;
    TokenType current_type;
    char current_char = lexer->source[lexer->pos];

    //end of file
    if (current_char == '\0')
        return returnEOF(lexer);

    //handle char literal content
    if (lexer->char_mode == true && lexer->char_filled == false) {
        Token tok = handleCharLiteral(
            lexer,
            &current_size);

        if (tok.type == TOKEN_CHAR)
            return tok;
    }

    //handle char literal errors
    if (lexer->char_mode == true && lexer->char_filled == true &&
        current_char != '\'' && current_char != '\\') //if filled 1 char (lexer->char_filled) but next char is not ' or \ and still in lexer->char_mode, error.
    {
        fprintf(stderr, "char literal takes too many args, line: %d, col: %d\n",
                lexer->line, lexer->col);
        exit(EXIT_FAILURE);
    }

    //handle quotes
    if (handleQuotes(lexer, &current_type)) {
        advance(lexer);
        return nextToken(lexer);
    }

    //handle string literals
    if (lexer->string_mode) {
        Token tok = handleString(lexer, &current_size);

        if (tok.type == TOKEN_STRING) {
            advance(lexer);
            return tok;
        }
    }
    //handle preprocessor directives

    //skip comments
    int comment_result = skipComment(lexer);
    if (comment_result == 1) {
        return (Token){
            .length = 1,
            .text = "\n",
            .type = TOKEN_PUNCTUATION};
    } else if (comment_result == 2) {
        return returnEOF(lexer);
    }

    //handle punctuation, operators, and numbers
    {
        Token tok = handlePunctuationOperatorNumber(
            lexer,
            &current_size,
            &current_type);

        if (tok.type == TOKEN_PUNCTUATION ||
            tok.type == TOKEN_OPERATOR) {
            advance(lexer);
            return tok;
        }
        if (tok.type == TOKEN_INTEGER || tok.type == TOKEN_FLOAT || tok.type == TOKEN_FIXED) //dont advance for number, it does that on its own.
        {
            return tok;
        }
    }

    //keyword or identifier.
    {
        Token tok = handleIdentifier(
            lexer,
            &current_size,
            &current_type);

        if (tok.type == TOKEN_KEYWORD ||
            tok.type == TOKEN_IDENTIFIER) {
            advance(lexer);
            return tok;
        }
    }

    advance(lexer);
    return (Token){
        .length = current_size,
        .text = lexer->current_text,
        .type = current_type};
}