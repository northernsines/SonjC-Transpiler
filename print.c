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
        case OPERATOR_SHIFT_LEFT:           return "<<";
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
        case OPERATOR_BRACKET:              return "[]";
        case OPERATOR_DOT:                  return ".";
        case OPERATOR_ARROW:                return "->";
        //STUB add more operators
    }
    return "unknown";
}

static const char *primitiveTypeToString(PrimitiveType type)
{
    switch (type)
    {
        case PRIMITIVE_INT:   return "int";
        case PRIMITIVE_CHAR:  return "char";
        case PRIMITIVE_FLOAT: return "float";
        case PRIMITIVE_VOID:  return "void";
        case PRIMITIVE_FIXED: return "fixed";
        case PRIMITIVE_BOOL:  return "bool";
        case PRIMITIVE_COUNT: break;
    }
    return "unknown";
}

static void printTypeInline(Node *type);

static void printQualifierSuffix(Node *qualifier)
{
    if (!qualifier || qualifier->type != NODE_TYPE_QUALIFIER) return;

    switch (qualifier->typeQualifier.kind)
    {
        case QUAL_POINTER:  printf("*"); break;
        case QUAL_CONST:    printf(" const"); break;
        case QUAL_VOLATILE: printf(" volatile"); break;
        case QUAL_UNSIGNED: printf(" unsigned"); break;
        case QUAL_RAW:      printf(" raw"); break;
        case QUAL_LONG:     printf(" long"); break;
        case QUAL_SHORT:    printf(" short"); break;
        case QUAL_ARRAY:    printf("[%d]", qualifier->typeQualifier.arraySize); break;
        case QUAL_FUNCTION:
            printf("(");
            for (int i = 0; i < qualifier->typeQualifier.funcQual.paramCount; i++)
            {
                if (i > 0) printf(", ");
                printTypeInline(qualifier->typeQualifier.funcQual.params[i]);
            }
            printf(")");
            break;
        default:
            printf("?");
            break;
    }
}

static void printTypeInline(Node *type)
{
    if (!type) return;

    switch (type->type)
    {
        case NODE_TYPE:
            printTypeInline(type->typeNode.innerType);
            printQualifierSuffix(type->typeNode.qualifier);
            break;
        case NODE_PRIMITIVE:
            printf("%s", primitiveTypeToString(type->primitiveType.type));
            break;
        case NODE_TYPEREF:
            printf("%s", type->typeRef.name);
            break;
        default:
            printf("<not a type>");
            break;
    }
}

void printNode(Node *node, int depth) //STUB implement all nodes
{
    if (!node) return;

    for (int i = 0; i < depth; i++) printf("____"); // indent per depth
    printf("|"); //marker

    switch (node->type)
    {
        case NODE_INT_LITERAL:
            printf("Int: %d\n", node->intLiteral.value);
            break;

        case NODE_FLOAT_LITERAL:
            printf("Float: %f\n", node->floatLiteral.value);
            break;

        case NODE_FIXED_LITERAL:
            printf("Fixed: %" PRIu32 "\n", node->fixedLiteral.value);
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

        case NODE_TYPE:
        case NODE_PRIMITIVE:
        case NODE_TYPEREF:
            printf("Type: ");
            printTypeInline(node);
            printf("\n");
            break;

        case NODE_TYPE_QUALIFIER:
            printf("Qualifier:");
            printQualifierSuffix(node);
            printf("\n");
            break;

        case NODE_TYPE_DECL:
            printf("Typedef: %s =", node->typeDecl.aliasName);
            if (node->typeDecl.underlyingInstance &&
                node->typeDecl.underlyingInstance->type == NODE_OBJECT_DECL)
            {
                printf("\n");
                printNode(node->typeDecl.underlyingInstance, depth + 1);
            }
            else
            {
                printf(" ");
                printTypeInline(node->typeDecl.underlyingInstance);
                printf("\n");
            }
            break;

        case NODE_OBJECT_DECL:
            printf("ObjectDecl: %s\n", node->objectDecl.name ? node->objectDecl.name : "<anonymous>");
            for (int i = 0; i < node->objectDecl.memberCount; i++)
                printNode(node->objectDecl.members[i], depth + 1);
            break;
        
        case NODE_VAR:
            printf("Var: %s\n", node->var.name);
            break;

        case NODE_VAR_DECL:
            printf("VarDecl\n");
            printNode(node->varDecl.type, depth + 1);
            printNode(node->varDecl.var, depth + 1);
            printNode(node->varDecl.valueExpr, depth + 1);
            break;

        case NODE_BINARY_EXPR:
            printf("binaryExp: %s\n", operatorTypeToString(node->binaryExpr.operator->operator.opType));
            printNode(node->binaryExpr.leftExp, depth + 1);
            printNode(node->binaryExpr.rightExp, depth + 1);
            break;

        case NODE_UNARY_EXPR:
            printf("unaryExp: %s\n", operatorTypeToString(node->unaryExpr.operator->operator.opType));
            printNode(node->unaryExpr.exp, depth + 1);
            break;

        case NODE_CALL_EXPR:
            printf("CallExpr\n");
            printNode(node->callExpr.callee, depth + 1);
            for (int i = 0; i < node->callExpr.argCount; i++)
                printNode(node->callExpr.args[i], depth + 1);
            break;

        case NODE_MEMBER_EXPR:
            printf("Member: %s%s\n", node->memberExpr.isArrow ? "->" : ".", node->memberExpr.member);
            printNode(node->memberExpr.object, depth + 1);
            break;

        case NODE_INDEX_EXPR:
            printf("Index\n");
            printNode(node->indexExpr.target, depth + 1);
            printNode(node->indexExpr.index, depth + 1);
            break;

        case NODE_TERNARY_EXPR:
            printf("Ternary: ?:\n");
            printNode(node->ternaryExpr.conditionExp, depth + 1);
            printNode(node->ternaryExpr.ifTrueExp, depth + 1);
            printNode(node->ternaryExpr.ifFalseExp, depth + 1);
            break;

        case NODE_CAST:
            printf("Cast\n");
            printNode(node->cast.type, depth + 1);
            printNode(node->cast.exp, depth + 1);
            break;

        case NODE_EXPR_STMT:
            printf("ExprStmt\n");
            printNode(node->exprStmt.expr, depth + 1);
            break;

        case NODE_IF_STMT:
            printf("IfStmt\n");
            printNode(node->ifStmt.expr, depth + 1);
            printNode(node->ifStmt.logic, depth + 1);
            if(!(node->ifStmt.elseStmt == NULL)) printNode(node->ifStmt.elseStmt, depth + 1);
            break;
        
        case NODE_ELSE_STMT:
            printf("ElseStmt\n");
            printNode(node->elseStmt.logic, depth + 1);
            break;

        case NODE_BLOCK:
            printf("Block\n");
            for (int i = 0; i < node->block.count; i++)
                printNode(node->block.statements[i], depth + 1);
            break;

        case NODE_FUNCTION_DECL:
            printf("FunctionDecl: %s returns ", node->functionDecl.name);
            printTypeInline(node->functionDecl.returnType);
            printf("\n");
            if (node->functionDecl.paramCount == 0)
            {
                for (int i = 0; i < depth + 1; i++) printf("____");
                printf("|");
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
                for (int j = 0; j < depth + 1; j++) printf("____");
                printf("|");
                printf("Import: %s\n", node->program.imports[i]);
            }
            for (int i = 0; i < node->program.globalCount; i++)
                printNode(node->program.globals[i], depth + 1);
            for (int i = 0; i < node->program.functionCount; i++)
                printNode(node->program.functions[i], depth + 1);
            break;

        default:
            printf("Unknown node type: %d\n", node->type);
    }
}