#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/*
SonjC Parser Utilities
Written Sep 2026
Utility functions for the parser: token/operator lookup, the operator rule
table, and error reporting. Split out of parser.c.
*/

static void vparserError(const char *message, va_list args)
{
    vfprintf(stderr, message, args);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

void parserError(const char *message, ...)
{
    va_list args;
    va_start(args, message);
    vparserError(message, args);
    va_end(args);
}

bool isTypeName(Token *t)
{ //STUB, expand with all type names in lang
    if(t->type != TOKEN_KEYWORD) return false;
    bool returnBool =
    !strcmp(t->text, "int") ||
    !strcmp(t->text, "float") ||
    !strcmp(t->text, "char") ||
    !strcmp(t->text, "bool") ||
    !strcmp(t->text, "string") ||
    !strcmp(t->text, "size_t");
    return returnBool;
}

const char *tokenTypeName(TokenType t)
{
    switch (t)
    {
        case TOKEN_IDENTIFIER:  return "identifier";
        case TOKEN_KEYWORD:     return "keyword";
        case TOKEN_PUNCTUATION: return "punctuation";
        case TOKEN_INTEGER:     return "integer";
        case TOKEN_FLOAT:       return "float";
        case TOKEN_FIXED:       return "fixed";
        case TOKEN_STRING:      return "string";
        case TOKEN_CHAR:        return "char";
        case TOKEN_EOF:         return "end of file";
        default:                return "unknown token";
    }
}

Token consume(Parser *p, TokenType expectedType, const char *expectedText, const char *context)
{
    if (p->pos > p->tokenCount)
        parserError("%s — reached end of file", context);

    Token tok = CUR(p);
    bool typeMatches = tok.type == expectedType;
    bool textMatches = (expectedText == NULL) || !strcmp(tok.text, expectedText);

    if (!typeMatches || !textMatches)
    { 
        if (expectedText)
            parserError("%s — expected '%s', got '%s' at position %d", context, expectedText, tok.text, p->pos);
        else
            parserError("%s — expected %s, got '%s' at position %d", context, tokenTypeName(expectedType), tok.text, p->pos);
    }

    p->pos++;
    return tok;
}

void skip(Parser *p, const char *context)
{
    if (p->pos > p->tokenCount) parserError("%s (reached end of file)", context);
    p->pos++;
}

int pow10(int num)
{
    if (num == 0) return 1;
    else return 10 * pow10(num - 1); //recursive exponentation
}

OperatorType lookupOperator(Token t)
{
    if (t.type != TOKEN_PUNCTUATION && t.type != TOKEN_KEYWORD && t.type != TOKEN_OPERATOR)
    return OPERATOR_NONE;

    if (!strcmp(t.text, ";") || !strcmp(t.text, "{"))  return OPERATOR_END_EXP;
    if (!strcmp(t.text, "+"))  return OPERATOR_ADD;
    if (!strcmp(t.text, "-"))  return OPERATOR_SUBTRACT;
    if (!strcmp(t.text, "*"))  return OPERATOR_MULTIPLY;
    if (!strcmp(t.text, "/"))  return OPERATOR_DIVIDE;
    if (!strcmp(t.text, "%"))  return OPERATOR_MOD;
    if (!strcmp(t.text, "="))  return OPERATOR_ASSIGN;
    if (!strcmp(t.text, "&&")) return OPERATOR_LOGICAL_AND;
    if (!strcmp(t.text, "||")) return OPERATOR_LOGICAL_OR;
    if (!strcmp(t.text, "&")) return OPERATOR_BITWISE_AND;
    if (!strcmp(t.text, "|")) return OPERATOR_BITWISE_OR;
    if (!strcmp(t.text, "^")) return OPERATOR_BITWISE_XOR;
    if (!strcmp(t.text, "==")) return OPERATOR_IS_EQUAL;
    if (!strcmp(t.text, "!=")) return OPERATOR_IS_NOT_EQUAL;
    if (!strcmp(t.text, "++")) return OPERATOR_INCREMENT;
    if (!strcmp(t.text, "--")) return OPERATOR_DECREMENT;
    if (!strcmp(t.text, "sizeof")) return OPERATOR_SIZEOF;
    if (!strcmp(t.text, "!")) return OPERATOR_LOGICAL_NOT;
    if (!strcmp(t.text, "~")) return OPERATOR_BITWISE_NOT;
    //STUB, add more operators

    return OPERATOR_NONE;
}

void initRules(void) // building the operator table
{
    rules[OPERATOR_END_EXP] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = -1,
            .nud = NULL,
        }
    };

    rules[OPERATOR_ASSIGN] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 20,
            .nud = NULL,
            .led = ledBinaryRight
        }
    };

    rules[OPERATOR_LOGICAL_OR] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 40,
            .nud = NULL,
            .led = ledBinaryLeft
        }
    };

    rules[OPERATOR_LOGICAL_AND] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 50,
            .nud = NULL,
            .led = ledBinaryLeft
        }
    };

    rules[OPERATOR_BITWISE_OR] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 60,
            .nud = NULL,
            .led = ledBinaryLeft
        }
    };

    rules[OPERATOR_BITWISE_XOR] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 70,
            .nud = NULL,
            .led = ledBinaryLeft
        }
    };

    rules[OPERATOR_BITWISE_AND] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 80,
            .nud = nudAddress,
            .led = ledBinaryLeft
        }
    };

    rules[OPERATOR_IS_EQUAL] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 90,
            .nud = NULL,
            .led = ledBinaryLeft
        }
    };

    rules[OPERATOR_IS_NOT_EQUAL] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 90,
            .nud = NULL,
            .led = ledBinaryLeft
        }
    };

    rules[OPERATOR_ADD] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 120,
            .nud = nudUnary,
            .led = ledBinaryLeft
        }
    };

    rules[OPERATOR_SUBTRACT] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 120,
            .nud = nudUnary,
            .led = ledBinaryLeft
        }
    };

    rules[OPERATOR_MULTIPLY] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 130,
            .nud = nudUnary,
            .led = ledBinaryLeft
        }
    };

    rules[OPERATOR_MOD] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 130,
            .nud = NULL,
            .led = ledBinaryLeft
        }
    };

    rules[OPERATOR_LOGICAL_NOT] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 140,
            .nud = nudUnary,
            .led = NULL,
        }
    };

    rules[OPERATOR_BITWISE_NOT] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 140,
            .nud = nudUnary,
            .led = NULL,
        }
    };

    rules[OPERATOR_INCREMENT] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 140,
            .nud = nudUnary,
            .led = ledUnary,
        }
    };

    rules[OPERATOR_DECREMENT] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 140,
            .nud = nudUnary,
            .led = ledUnary,
        }
    };

    rules[OPERATOR_SIZEOF] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 140,
            .nud = nudUnary,
            .led = NULL,
        }
    };

    //STUB continue adding operator rules
}