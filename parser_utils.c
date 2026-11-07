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

bool primitiveFromToken(Token *t, PrimitiveType *out)
{ //expand with all PRIMITIVE type names in lang
    if(t->type != TOKEN_KEYWORD) return false;

    PrimitiveType primitive;
    if      (!strcmp(t->text, "int"))   primitive = PRIMITIVE_INT;
    else if (!strcmp(t->text, "char"))  primitive = PRIMITIVE_CHAR;
    else if (!strcmp(t->text, "float")) primitive = PRIMITIVE_FLOAT;
    else if (!strcmp(t->text, "void"))  primitive = PRIMITIVE_VOID;
    else if (!strcmp(t->text, "fixed")) primitive = PRIMITIVE_FIXED;
    else if (!strcmp(t->text, "bool"))  primitive = PRIMITIVE_BOOL;
    else return false;

    if (out) *out = primitive;
    return true;
}

static const PrimitiveInfo primitiveTable[PRIMITIVE_COUNT] = {
    [PRIMITIVE_INT]   = { .bitWidth = 32, .isSigned = true,  .fracBits = 0  },
    [PRIMITIVE_CHAR]  = { .bitWidth = 8,  .isSigned = false, .fracBits = 0  },
    [PRIMITIVE_FLOAT] = { .bitWidth = 32, .isSigned = true,  .fracBits = 0  },
    [PRIMITIVE_VOID]  = { .bitWidth = 0,  .isSigned = false, .fracBits = 0  },
    [PRIMITIVE_FIXED] = { .bitWidth = 32, .isSigned = true,  .fracBits = 16 },
    [PRIMITIVE_BOOL]  = { .bitWidth = 8,  .isSigned = false, .fracBits = 0  },
};

const PrimitiveInfo *primitiveInfoFor(PrimitiveType type) //metadata for primitive widths and signedness
{
    if (type < 0 || type >= PRIMITIVE_COUNT) return NULL;
    return &primitiveTable[type];
}

bool isTypeName(Parser *p, Token *t)
{
    if (primitiveFromToken(t, NULL)) return true;

    if (t->type == TOKEN_IDENTIFIER && p != NULL)
    {
        for (int i = 0; i < p->defTypeCount; i++)
            if (!strcmp(p->defTypes[i], t->text)) return true;
    }

    return false;
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
    if (p->pos >= p->tokenCount)
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
    if (p->pos >= p->tokenCount) parserError("%s (reached end of file)", context);
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
    if (!strcmp(t.text, ","))  return OPERATOR_COMMA;
    if (!strcmp(t.text, "+"))  return OPERATOR_ADD;
    if (!strcmp(t.text, "-"))  return OPERATOR_SUBTRACT;
    if (!strcmp(t.text, "*"))  return OPERATOR_MULTIPLY;
    if (!strcmp(t.text, "/"))  return OPERATOR_DIVIDE;
    if (!strcmp(t.text, "%"))  return OPERATOR_MOD;
    if (!strcmp(t.text, "="))  return OPERATOR_ASSIGN;
    if (!strcmp(t.text, "+=")) return OPERATOR_ASSIGN_ADD;
    if (!strcmp(t.text, "-=")) return OPERATOR_ASSIGN_SUBTRACT;
    if (!strcmp(t.text, "*=")) return OPERATOR_ASSIGN_MULTIPLY;
    if (!strcmp(t.text, "/=")) return OPERATOR_ASSIGN_DIVIDE;
    if (!strcmp(t.text, "%=")) return OPERATOR_ASSIGN_MOD;
    if (!strcmp(t.text, "&=")) return OPERATOR_BITWISE_ASSIGN_AND;
    if (!strcmp(t.text, "|=")) return OPERATOR_BITWISE_ASSIGN_OR;
    if (!strcmp(t.text, "^=")) return OPERATOR_BITWISE_ASSIGN_XOR;
    if (!strcmp(t.text, "<<=")) return OPERATOR_BITWISE_ASSIGN_LEFTSHIFT;
    if (!strcmp(t.text, ">>=")) return OPERATOR_BITWISE_ASSIGN_RIGHTSHIFT;
    if (!strcmp(t.text, "?")) return OPERATOR_TERNARY;
    if (!strcmp(t.text, "&&")) return OPERATOR_LOGICAL_AND;
    if (!strcmp(t.text, "||")) return OPERATOR_LOGICAL_OR;
    if (!strcmp(t.text, "&")) return OPERATOR_BITWISE_AND;
    if (!strcmp(t.text, "(")) return OPERATOR_CAST;
    if (!strcmp(t.text, "|")) return OPERATOR_BITWISE_OR;
    if (!strcmp(t.text, "^")) return OPERATOR_BITWISE_XOR;
    if (!strcmp(t.text, "==")) return OPERATOR_IS_EQUAL;
    if (!strcmp(t.text, "!=")) return OPERATOR_IS_NOT_EQUAL;
    if (!strcmp(t.text, ">")) return OPERATOR_GREATER_THAN;
    if (!strcmp(t.text, "<")) return OPERATOR_LESS_THAN;
    if (!strcmp(t.text, ">=")) return OPERATOR_GREATER_THAN_OREQ;
    if (!strcmp(t.text, "<=")) return OPERATOR_LESS_THAN_OREQ;
    if (!strcmp(t.text, "<<")) return OPERATOR_SHIFT_LEFT;
    if (!strcmp(t.text, ">>")) return OPERATOR_SHIFT_RIGHT;    
    if (!strcmp(t.text, "++")) return OPERATOR_INCREMENT;
    if (!strcmp(t.text, "--")) return OPERATOR_DECREMENT;
    if (!strcmp(t.text, "sizeof")) return OPERATOR_SIZEOF;
    if (!strcmp(t.text, "!")) return OPERATOR_LOGICAL_NOT;
    if (!strcmp(t.text, "~")) return OPERATOR_BITWISE_NOT;
    if (!strcmp(t.text, "[")) return OPERATOR_BRACKET;
    if (!strcmp(t.text, ".")) return OPERATOR_DOT;
    if (!strcmp(t.text, "->")) return OPERATOR_ARROW;
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

    rules[OPERATOR_COMMA] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 10,
            .nud = NULL,
            .led = ledBinaryLeft
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

    rules[OPERATOR_ASSIGN_ADD] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 20,
            .nud = NULL,
            .led = ledBinaryRight
        }
    };

    rules[OPERATOR_ASSIGN_SUBTRACT] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 20,
            .nud = NULL,
            .led = ledBinaryRight
        }
    };

    rules[OPERATOR_ASSIGN_MULTIPLY] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 20,
            .nud = NULL,
            .led = ledBinaryRight
        }
    };

    rules[OPERATOR_ASSIGN_DIVIDE] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 20,
            .nud = NULL,
            .led = ledBinaryRight
        }
    };

    rules[OPERATOR_ASSIGN_MOD] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 20,
            .nud = NULL,
            .led = ledBinaryRight
        }
    };

    rules[OPERATOR_BITWISE_ASSIGN_AND] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 20,
            .nud = NULL,
            .led = ledBinaryRight
        }
    };

    rules[OPERATOR_BITWISE_ASSIGN_OR] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 20,
            .nud = NULL,
            .led = ledBinaryRight
        }
    };

    rules[OPERATOR_BITWISE_ASSIGN_XOR] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 20,
            .nud = NULL,
            .led = ledBinaryRight
        }
    };

    rules[OPERATOR_BITWISE_ASSIGN_LEFTSHIFT] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 20,
            .nud = NULL,
            .led = ledBinaryRight
        }
    };

    rules[OPERATOR_BITWISE_ASSIGN_RIGHTSHIFT] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 20,
            .nud = NULL,
            .led = ledBinaryRight
        }
    };

    rules[OPERATOR_TERNARY] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 30,
            .nud = NULL,
            .led = ledTernary
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

    rules[OPERATOR_GREATER_THAN] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 100,
            .nud = NULL,
            .led = ledBinaryLeft
        }
    };

    rules[OPERATOR_LESS_THAN] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 100,
            .nud = NULL,
            .led = ledBinaryLeft
        }
    };

    rules[OPERATOR_LESS_THAN_OREQ] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 100,
            .nud = NULL,
            .led = ledBinaryLeft
        }
    };

    rules[OPERATOR_GREATER_THAN_OREQ] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 100,
            .nud = NULL,
            .led = ledBinaryLeft
        }
    };

    rules[OPERATOR_SHIFT_LEFT] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 110,
            .nud = NULL,
            .led = ledBinaryLeft
        }
    };

    rules[OPERATOR_SHIFT_RIGHT] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 110,
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

    rules[OPERATOR_DIVIDE] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 130,
            .nud = NULL,
            .led = ledBinaryLeft
        }
    };

    rules[OPERATOR_CAST] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 150,
            .nud = nudParen,
            .led = ledCall,
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
            .nud = nudSizeof,
            .led = NULL,
        }
    };

    rules[OPERATOR_BRACKET] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 150,
            .nud = NULL,
            .led = ledIndex,
        }
    };

    rules[OPERATOR_DOT] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 150,
            .nud = NULL,
            .led = ledMember,
        }
    };

    rules[OPERATOR_ARROW] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 150,
            .nud = NULL,
            .led = ledMember,
        }
    };

    //STUB continue adding operator rules
}