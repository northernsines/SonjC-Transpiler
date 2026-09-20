#include <stdio.h>    
#include <stdlib.h>   
#include <string.h> 
#include "lexer.h"

/*
SonjC Lexer
Written Aug 2026
Converts C source code into tokens to be used by the SonjC transpiler.
*/

void advance(Lexer *lexer)
{
    lexer->col++;
    lexer->pos++;
}

void retreat(Lexer *lexer)
{
    lexer->col--;
    lexer->pos--;
}

void newLine(Lexer *lexer)
{
    lexer->col = 0;
    lexer->line++;
}

Token returnEOF(Lexer* lexer)
{
    *lexer->currentText = '\0';
    return (Token){
        .length = 1,
        .text = lexer->currentText,
        .type = TOKEN_EOF
    };
}

bool isNumber(char currentChar)
{
    return ('0' <= currentChar && currentChar <= '9');
}

bool isKeyword(char *text)
{
    return
        // standard C types/qualifiers
        !strcmp(text, "int") ||
        !strcmp(text, "float") ||
        !strcmp(text, "double") ||
        !strcmp(text, "char") ||
        !strcmp(text, "bool") ||
        !strcmp(text, "void") ||
        !strcmp(text, "short") ||
        !strcmp(text, "long") ||
        !strcmp(text, "signed") ||
        !strcmp(text, "unsigned") ||
        !strcmp(text, "const") ||
        !strcmp(text, "volatile") ||
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
        !strcmp(text, "struct") ||
        !strcmp(text, "union") ||
        !strcmp(text, "typedef") ||
        !strcmp(text, "sizeof") ||
        !strcmp(text, "size_t") ||
        // GNU/common extension
        !strcmp(text, "typeof") ||
        !strcmp(text, "asm") ||
        // SonjC-specific additions
        !strcmp(text, "elif") ||
        !strcmp(text, "fixed") ||
        !strcmp(text, "iterate") ||
        !strcmp(text, "string") ||
        !strcmp(text, "object") ||
        !strcmp(text, "public") ||
        !strcmp(text, "import") ||
        !strcmp(text, "private");
}

bool isOperator (char currentChar) //list of operators
{
    return
        currentChar == '+' ||
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
        currentChar == '$' ||
        currentChar == '.';
}

bool isMulticharOperator(char *text)
{
    return
        // string operators (SonjC)
        !strcmp(text, "$==") ||
        !strcmp(text, "$=") ||
        !strcmp(text, "$+") ||
        // three-char
        !strcmp(text, "<<=") ||
        !strcmp(text, ">>=") ||
        !strcmp(text, "...") ||
        // two-char
        !strcmp(text, "&&") ||
        !strcmp(text, "&=") ||
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

bool isPunctuation (char currentChar) //list of punctuation
{
    return
        currentChar == '{' ||
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

void failIfOver (int size, Lexer *lexer)
{
    if (size >= 299)
    {
        fprintf(stderr, "too long of identifier!, line: %d, col %d\n", lexer->line, lexer->col); 
        exit(EXIT_FAILURE);
    }
    else return;
}

void skipNewline(Lexer *lexer)
{
    newLine(lexer);
    advance(lexer);
}

int skipComment(Lexer *lexer)
{
    char currentChar = lexer->source[lexer->pos];

    if (currentChar != '/') return 0;
    if(currentChar == '\0') return 2;

    advance(lexer);
    currentChar = lexer->source[lexer->pos];

    if (currentChar == '/')
    {
        while(currentChar != '\n')
        {
            if(currentChar == '\0') return 2;
            currentChar = lexer->source[lexer->pos];
            advance(lexer);
        }

        skipNewline(lexer);
        return 1;
    }
    else if (currentChar == '*')
    {
        while (1)
        {
            currentChar = lexer->source[lexer->pos];

            if (currentChar == '\0')
                return 2;

            if (currentChar == '\n')
                newLine(lexer);

            if (currentChar == '*' &&
                lexer->source[lexer->pos + 1] == '/')
            {
                advance(lexer); // consume '*'
                advance(lexer); // consume '/'
                return 1;
            }

            advance(lexer);
        }
    }
    else
    {
        retreat(lexer);
        return 0;
    }
}

Token handleCharLiteral(Lexer *lexer, int *currentSize)
{
    char currentChar = lexer->source[lexer->pos];

    if(lexer->charMode == true && lexer->charFilled == true &&
       currentChar != '\'' && currentChar != '\\') //if filled 1 char (lexer->charFilled) but next char is not ' or \ and still in lexer->charMode, error.
    {
        fprintf(stderr, "char literal takes too many args, line: %d, col %d\n",
                lexer->line, lexer->col);
        exit(EXIT_FAILURE);
    }

    if(lexer->charMode == true && lexer->charFilled == false) //handle char itself
    {
        if(currentChar == '\0') return returnEOF(lexer);
        if(currentChar == '\\') //if \, keep as raw text, grab next char too
        {
            lexer->currentText[*currentSize] = currentChar;
            (*currentSize)++;
            if(currentChar == '\0') return returnEOF(lexer);
            advance(lexer);
            currentChar = lexer->source[lexer->pos];
        }

        lexer->currentText[*currentSize] = currentChar;
        (*currentSize)++;
        if(currentChar == '\0') return returnEOF(lexer);
        lexer->charFilled = true;
        advance(lexer);

        return (Token){
            .length = *currentSize,
            .text = lexer->currentText,
            .type = TOKEN_CHAR
        };
    }

    return (Token){0};
}

bool handleQuotes(Lexer *lexer, TokenType *currentType)
{
    char currentChar = lexer->source[lexer->pos];
    *currentType = TOKEN_STRING;

    if(currentChar == '\'') //handle single quotes themselves
    {
        if(lexer->charMode == false) lexer->charMode = true;
        else
        {
            lexer->charMode = false;
            lexer->charFilled = false;
        }
        return true;     // signals "a quote was consumed, caller should re-lex"
    }

    if(currentChar == '"') //handle double quotes themselves
    {
        if(lexer->stringMode == false) lexer->stringMode = true;
        else lexer->stringMode = false;
        return true;
    }

    return false;
}

Token handleString(Lexer *lexer, int *currentSize)
{
    char currentChar = lexer->source[lexer->pos];

    if(lexer->stringMode)
    {
        while(currentChar != '"')
        {
            if(currentChar == '\0') return returnEOF(lexer);
            if(currentChar == '\\')
            {
                lexer->currentText[*currentSize] = currentChar;
                (*currentSize)++;
                advance(lexer);
                currentChar = lexer->source[lexer->pos];
            }

            lexer->currentText[*currentSize] = currentChar;
            (*currentSize)++;
            failIfOver(*currentSize, lexer);
            advance(lexer);
            currentChar = lexer->source[lexer->pos];
        }

        lexer->currentText[*currentSize] = '\0';
        lexer->stringMode = false; // closing quote found, exit string mode

        return (Token){
            .length = *currentSize,
            .text = lexer->currentText,
            .type = TOKEN_STRING
        };
    }

    return (Token){0};
}

Token readNumber(Lexer *lexer, int *currentSize)
{
    char currentChar = lexer->source[lexer->pos];
    bool seenDot = false;
    bool seenF = false;

    while((currentChar >= '0' && currentChar <= '9') || (currentChar == '.' && seenDot == false) || (currentChar >= 'f' && seenF == false)) //digit and single decimal is allowed, as well as f for fixed marker.
    {
        lexer->currentText[*currentSize] = currentChar;
        (*currentSize)++;
        failIfOver(*currentSize, lexer);
        if(currentChar == '.') seenDot = true;
        if(currentChar == 'f') seenF = true;
        advance(lexer); //next pos
        currentChar = lexer->source[lexer->pos];
    }

    TokenType numberType = TOKEN_INTEGER; //int by default
    if (seenDot) numberType = TOKEN_FLOAT; //float by default if point detected
    if (seenF) numberType = TOKEN_FIXED; //fixed if fixed specifier detected
    lexer->currentText[*currentSize] = '\0';
    
    return(Token) {
        .length = *currentSize,
        .text = lexer->currentText,
        .type = numberType
    };
}

void readOperator(Lexer *lexer, int *currentSize)
{
    char currentChar = lexer->source[lexer->pos];

    advance(lexer); //next pos
    currentChar = lexer->source[lexer->pos];

    if(isOperator(currentChar))
    {
        advance(lexer); //next pos
        lexer->currentText[1] = currentChar;
        currentChar = lexer->source[lexer->pos];
        (*currentSize)++;

        if(isOperator(currentChar)) //triple char operator!
        {
            lexer->currentText[2] = currentChar;
            lexer->currentText[3] = '\0';

            if(!isMulticharOperator(lexer->currentText))
            {
                fprintf(stderr, "invalid multichar operator, line: %d, col %d\n",
                        lexer->line, lexer->col);
                exit(EXIT_FAILURE);
            }

            (*currentSize)++;
        }
        else //double char operator
        {
            retreat(lexer);
            lexer->currentText[2] = '\0';

            if(!isMulticharOperator(lexer->currentText))
            {
                fprintf(stderr, "invalid multichar operator, line: %d, col %d\n",
                        lexer->line, lexer->col);
                exit(EXIT_FAILURE);
            }
        }
    }
    else
    {
        retreat(lexer);
    }
}

Token handlePunctuationOperatorNumber(Lexer *lexer, int *currentSize, TokenType *currentType)
{
    char currentChar = lexer->source[lexer->pos];

    if(isPunctuation(currentChar) ||
       isOperator(currentChar) ||
       isNumber(currentChar))
    {
        if(isPunctuation(currentChar)) *currentType = TOKEN_PUNCTUATION;
        else if(isOperator(currentChar))
        {
            *currentType = TOKEN_OPERATOR;
        }
        if(currentChar == '\n') newLine(lexer); //new line if newline

        lexer->currentText[0] = currentChar;
        (*currentSize)++;

        if(isOperator(currentChar)) //check multichar operator
        {
            readOperator(lexer, currentSize);
        }
        if(isNumber(currentChar)) //load multichar Number
        {
            advance(lexer);
            return readNumber(lexer, currentSize);
        }

        lexer->currentText[*currentSize] = '\0'; //null terminate
        return (Token){
            .length = *currentSize,
            .text = lexer->currentText,
            .type = *currentType
        };
    }

    return (Token){0};
}

Token handleDirective(Lexer *lexer, int *currentSize, TokenType *currentType)
{
    *currentType = TOKEN_DIRECTIVE;
    char currentChar = lexer->source[lexer->pos];
    while (currentChar != '\n') //start at # and lex everything from then to newline as pp directive
    {
        if(currentChar == '\0') return returnEOF(lexer);
        lexer->currentText[*currentSize] = currentChar;
        advance(lexer);
        (*currentSize)++;
        currentChar = lexer->source[lexer->pos];
    }
    return (Token){
    .length = *currentSize,
    .text = lexer->currentText,
    .type = *currentType
    };
}

Token handleIdentifier(Lexer *lexer, int *currentSize, TokenType *currentType)
{
    char currentChar = lexer->source[lexer->pos];

    if(!isPunctuation(currentChar) &&
       !isOperator(currentChar) &&
       currentChar != '\'' &&
       currentChar != '"')
    {
        while(!isPunctuation(currentChar) &&
              !isOperator(currentChar) &&
              currentChar != '\'' &&
              currentChar != '"')
        {
            lexer->currentText[*currentSize] = currentChar;
            (*currentSize)++;
            failIfOver(*currentSize, lexer);
            advance(lexer); //next pos
            currentChar = lexer->source[lexer->pos];
        }

        lexer->currentText[*currentSize] = '\0'; //add null terminator so strmcp works
        retreat(lexer); //back 1 position to set up next char as ending punct

        if(isKeyword(lexer->currentText)) *currentType = TOKEN_KEYWORD;
        else *currentType = TOKEN_IDENTIFIER;

        return (Token){
            .length = *currentSize,
            .text = lexer->currentText,
            .type = *currentType
        };
    }

    return (Token){0};
}

Token nextToken(Lexer *lexer)
{
    int currentSize = 0;
    TokenType currentType;
    char currentChar = lexer->source[lexer->pos];

    //end of file
    if(currentChar == '\0') return returnEOF(lexer);

    //handle char literal content
    if(lexer->charMode == true && lexer->charFilled == false)
    {
        Token tok = handleCharLiteral(
            lexer,
            &currentSize
        );

        if(tok.type == TOKEN_CHAR)
            return tok;
    }

    //handle char literal errors
    if(lexer->charMode == true && lexer->charFilled == true &&
       currentChar != '\'' && currentChar != '\\') //if filled 1 char (lexer->charFilled) but next char is not ' or \ and still in lexer->charMode, error.
    {
        fprintf(stderr, "char literal takes too many args, line: %d, col: %d\n",
                lexer->line, lexer->col);
        exit(EXIT_FAILURE);
    }

    //handle quotes
    if(handleQuotes(lexer, &currentType))
    {
        advance(lexer);
        return nextToken(lexer);
    }

    //handle string literals
    if(lexer->stringMode)
    {
        Token tok = handleString(lexer, &currentSize);

        if(tok.type == TOKEN_STRING)
        {
            advance(lexer);
            return tok;
        }
    }
    //handle preprocessor directives
    if(currentChar == '#')
    {
        Token tok = handleDirective(lexer, &currentSize, &currentType);
            return tok;
    }

    //skip comments
    int commentResult = skipComment(lexer);
    if(commentResult == 1)
    {
        return (Token){
            .length = 1,
            .text = "\n",
            .type = TOKEN_PUNCTUATION
        };
    }
    else if(commentResult == 2)
    {
        return returnEOF(lexer);
    }

    //handle punctuation, operators, and numbers
    {
        Token tok = handlePunctuationOperatorNumber(
            lexer,
            &currentSize,
            &currentType
        );

        if(tok.type == TOKEN_PUNCTUATION ||
           tok.type == TOKEN_OPERATOR)
        {
            advance(lexer);
            return tok;
        }
        if(tok.type == TOKEN_INTEGER || tok.type == TOKEN_FLOAT || tok.type == TOKEN_FIXED) //dont advance for number, it does that on its own.
        {
            return tok;
        }
    }

    //keyword or identifier.
    {
        Token tok = handleIdentifier(
            lexer,
            &currentSize,
            &currentType
        );

        if(tok.type == TOKEN_KEYWORD ||
           tok.type == TOKEN_IDENTIFIER)
        {
            advance(lexer);
            return tok;
        }
    }

    advance(lexer);
    return (Token){
        .length = currentSize,
        .text = lexer->currentText,
        .type = currentType
    };
}