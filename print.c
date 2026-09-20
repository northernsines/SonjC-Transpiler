#include "print.h"
#include <stdio.h>
#include <inttypes.h>

/*
SonjC Print
Written Sep 2026
Debug output helpers for token streams and the abstract syntax tree.
*/

void printTokenStream(Token *tokens, int count)
{
    for (int i = 0; i < count; i++)
    {
        printf("[%02d] type=%s text: %s\n", i, tokentypeToString(tokens[i].type), tokens[i].text);
    }
    fflush(stdout);
}

const char *tokentypeToString(TokenType type)
{
    return
        type == TOKEN_KEYWORD ? "KEYW" :
        type == TOKEN_IDENTIFIER ? "IDEN" :
        type == TOKEN_PUNCTUATION ? "PUNC" :
        type == TOKEN_OPERATOR ? "OPER" :
        type == TOKEN_STRING ? "STRN" :
        type == TOKEN_INTEGER ? "INTG" :
        type == TOKEN_FLOAT ? "FLOT" :
        type == TOKEN_FIXED ? "FIXD" :
        type == TOKEN_DIRECTIVE ? "DIRV":
        type == TOKEN_NONE ? "NONE":
        type == TOKEN_EOF ? "EOFL":
        type == TOKEN_CHAR ? "CHAR" : "UKWN";
}

const char *operatorTypeToString(OperatorType op)
{
    switch (op)
    {
        case OPERATOR_END_EXP:              return "endExp";
        case OPERATOR_NONE:                 return "none";
        case OPERATOR_COMMA:                return ",";
        case OPERATOR_ASSIGN:               return "=";
        case OPERATOR_ASSIGN_ADD:           return "+=";
        case OPERATOR_ASSIGN_SUBTRACT:      return "-=";
        case OPERATOR_ASSIGN_MULTIPLY:      return "*=";
        case OPERATOR_ASSIGN_DIVIDE:        return "/=";
        case OPERATOR_ASSIGN_MOD:           return "%=";
        case OPERATOR_BITWISE_ASSIGN_AND:   return "&=";
        case OPERATOR_BITWISE_ASSIGN_OR:    return "|=";
        case OPERATOR_BITWISE_ASSIGN_XOR:   return "^=";
        case OPERATOR_BITWISE_ASSIGN_LEFTSHIFT:  return "<<=";
        case OPERATOR_BITWISE_ASSIGN_RIGHTSHIFT: return ">>=";
        case OPERATOR_TERNARY:              return "?";
        case OPERATOR_LOGICAL_OR:           return "||";
        case OPERATOR_LOGICAL_AND:          return "&&";
        case OPERATOR_BITWISE_AND:          return "&";
        case OPERATOR_BITWISE_OR:           return "|";
        case OPERATOR_BITWISE_XOR:          return "^";
        case OPERATOR_IS_EQUAL:             return "==";
        case OPERATOR_IS_NOT_EQUAL:         return "!=";
        case OPERATOR_GREATER_THAN:         return ">";
        case OPERATOR_LESS_THAN:            return "<";
        case OPERATOR_LESS_THAN_OREQ:       return "<=";
        case OPERATOR_GREATER_THAN_OREQ:    return ">=";
        case OPERATOR_SHIFT_LOW:            return "<<";
        case OPERATOR_SHIFT_RIGHT:          return ">>";
        case OPERATOR_ADD:                  return "+";
        case OPERATOR_SUBTRACT:             return "-";
        case OPERATOR_MULTIPLY:             return "*";
        case OPERATOR_DIVIDE:               return "/";
        case OPERATOR_MOD:                  return "%";
        case OPERATOR_CAST:                 return "cast";
        case OPERATOR_LOGICAL_NOT:          return "!";
        case OPERATOR_BITWISE_NOT:          return "~";
        case OPERATOR_SIZEOF:               return "sizeof";
        case OPERATOR_INCREMENT:            return "++";
        case OPERATOR_DECREMENT:            return "--";
        case OPERATOR_PARENTHESIS:          return "()";
        case OPERATOR_BRACKET:              return "[]";
        case OPERATOR_DOT:                  return ".";
        case OPERATOR_ARROW:                return "->";
    }
    return "unknown";
}

void printNode(Node *node, int depth)
{
    if (!node) return;

    for (int i = 0; i < depth; i++) printf("  "); // indent per depth

    switch (node->type)
    {
        case NODE_INT_LITERAL:
            printf("Int: %d\n", node->intLiteral.value);
            break;

        case NODE_FLOAT_LITERAL:
            printf("Float: %f\n", node->floatLiteral.value);
            break;

        case NODE_FIXED_LITERAL:
            printf("Fixed: %" PRIu64 "\n", node->fixedLiteral.value);
            break;

        case NODE_CHAR_LITERAL:
            printf("Char: %c \n", node->charLiteral.value);
            break;

        case NODE_STRING_LITERAL:
            printf("String: %s\n", node->stringLiteral.value);
            break;
        
        case NODE_BOOL_LITERAL:
            printf("Bool: %s\n", node->boolLiteral.value ? "true" : "false");
            break;
        
        case NODE_VAR:
            printf("Var: %s\n", node->var.name);
            break;

        case NODE_VAR_DECL:
            printf("VarDecl: %s\n", node->varDecl.name);
            if(node->varDecl.value != NULL) printNode(node->varDecl.value, depth + 1);
            break;

        case NODE_BINARY_EXPR:
            printf("binaryExp: %s\n", operatorTypeToString(node->binaryExpr.operator->operator.opType));
            printNode(node->binaryExpr.leftExp, depth + 1);
            printNode(node->binaryExpr.rightExp, depth + 1);
            break;

        case NODE_CALL_EXPR:
            printf("CallExpr: %s\n", node->callExpr.name);
            for (int i = 0; i < node->callExpr.argCount; i++)
                printNode(node->callExpr.args[i], depth + 1);
            break;

        case NODE_EXPR_STMT:
            printf("ExprStmt\n");
            printNode(node->exprStmt.expr, depth + 1);
            break;

        case NODE_BLOCK:
            printf("Block\n");
            for (int i = 0; i < node->block.count; i++)
                printNode(node->block.statements[i], depth + 1);
            break;

        case NODE_FUNCTION_DECL:
            printf("FunctionDecl: %s returns %s\n",
                   node->functionDecl.name,
                   node->functionDecl.returnType);
            if (node->functionDecl.paramCount == 0)
            {
                for (int i = 0; i < depth + 1; i++) printf("  ");
                printf("void\n");
            }
            else
            {
                for (int i = 0; i < node->functionDecl.paramCount; i++)
                    printNode(node->functionDecl.params[i], depth + 1);
            }
            printNode(node->functionDecl.body, depth + 1);
            break;

        case NODE_PROGRAM:
            printf("Program\n");
            for (int i = 0; i < node->program.importCount; i++)
            {
                for (int j = 0; j < depth + 1; j++) printf("  ");
                printf("Import: %s\n", node->program.imports[i]);
            }
            for (int i = 0; i < node->program.functionCount; i++)
                printNode(node->program.functions[i], depth + 1);
            break;

        default:
            printf("Unknown node type: %d\n", node->type);
    }
}