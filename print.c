#include "print.h"
#include <stdio.h>
#include <inttypes.h>
#include <stdarg.h>
#include <string.h>

/*
SonjC Print
Written Sep 2026
Debug output helpers for token streams and the abstract syntax tree.
*/

#define TOKEN_TEXT_WIDTH_MAX 48

static void formatTokenText(const char *text, char *out, size_t outSize)
{
    if (outSize == 0) return;
    if (!text) text = "";

    size_t oi = 0;
    bool truncated = false;

    for (size_t i = 0; text[i] != '\0'; i++)
    {
        char single[2] = { text[i], '\0' };
        const char *rep;

        switch (text[i])
        {
            case '\n': rep = "\\n"; break;
            case '\t': rep = "\\t"; break;
            case '\r': rep = "\\r"; break;
            default:   rep = single; break;
        }

        size_t rl = strlen(rep);
        if (oi + rl + 1 > outSize)
        {
            truncated = true;
            break;
        }

        memcpy(out + oi, rep, rl);
        oi += rl;
    }

    if (truncated && outSize >= 4)
    {
        size_t keep = outSize - 4;
        if (oi > keep) oi = keep;
        memcpy(out + oi, "...", 3);
        oi += 3;
    }

    out[oi] = '\0';
}

static void printHorizontalRule(const char *left, const char *mid, const char *right,
                                int idxWidth, int typeWidth, int textWidth)
{
    printf("%s", left);
    for (int i = 0; i < idxWidth + 2; i++) printf("─");
    printf("%s", mid);
    for (int i = 0; i < typeWidth + 2; i++) printf("─");
    printf("%s", mid);
    for (int i = 0; i < textWidth + 2; i++) printf("─");
    printf("%s\n", right);
}

void printTokenStream(Token *tokens, int count)
{
    int idxWidth = 3;
    if (count > 0)
    {
        int digits = snprintf(NULL, 0, "%d", count - 1);
        if (digits > idxWidth) idxWidth = digits;
    }

    int typeWidth = 4;
    int textWidth = 4;

    for (int i = 0; i < count; i++)
    {
        char display[TOKEN_TEXT_WIDTH_MAX + 1];
        formatTokenText(tokens[i].text, display, sizeof(display));

        int len = (int)strlen(display);
        if (len > textWidth) textWidth = len;
    }

    printHorizontalRule("┌", "┬", "┐", idxWidth, typeWidth, textWidth);
    printf("│ %-*s │ %-*s │ %-*s │\n", idxWidth, "idx", typeWidth, "type", textWidth, "text");
    printHorizontalRule("├", "┼", "┤", idxWidth, typeWidth, textWidth);

    for (int i = 0; i < count; i++)
    {
        char display[TOKEN_TEXT_WIDTH_MAX + 1];
        formatTokenText(tokens[i].text, display, sizeof(display));

        printf("│ %*d │ %-*s │ %-*s │\n",
               idxWidth, i,
               typeWidth, tokentypeToString(tokens[i].type),
               textWidth, display);
    }

    printHorizontalRule("└", "┴", "┘", idxWidth, typeWidth, textWidth);
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

#define AST_PREFIX_MAX 1024

static void printNodeAt(Node *node, const char *prefix, bool isLast, bool isRoot);

static void printLeafLine(const char *prefix, bool isLast, const char *fmt, ...)
{
    va_list args;

    printf("%s%s ", prefix, isLast ? "└──" : "├──");
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

static void printChildArray(Node **children, int count, const char *prefix)
{
    int total = 0;
    for (int i = 0; i < count; i++)
        if (children[i]) total++;

    int seen = 0;
    for (int i = 0; i < count; i++)
    {
        if (!children[i]) continue;
        seen++;
        printNodeAt(children[i], prefix, seen == total, false);
    }
}

#define PRINT_CHILDREN(prefix, ...)                                                  \
    do {                                                                             \
        Node *_children[] = { __VA_ARGS__ };                                         \
        printChildArray(_children, (int)(sizeof(_children) / sizeof(_children[0])),  \
                        (prefix));                                                   \
    } while (0)

static void printNodeAt(Node *node, const char *prefix, bool isLast, bool isRoot)
{
    if (!node) return;

    if (!isRoot)
        printf("%s%s ", prefix, isLast ? "└──" : "├──");

    char childPrefix[AST_PREFIX_MAX];
    if (isRoot)
    {
        childPrefix[0] = '\0';
    }
    else
    {
        snprintf(childPrefix, sizeof(childPrefix), "%s%s", prefix, isLast ? "    " : "│   ");
    }

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
                printNodeAt(node->typeDecl.underlyingInstance, childPrefix, true, false);
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
            printChildArray(node->objectDecl.members, node->objectDecl.memberCount, childPrefix);
            break;

        case NODE_VAR:
            printf("Var: %s\n", node->var.name);
            break;

        case NODE_VAR_DECL:
            printf("VarDecl\n");
            PRINT_CHILDREN(childPrefix, node->varDecl.type, node->varDecl.var, node->varDecl.valueExpr);
            break;

        case NODE_BINARY_EXPR:
            printf("binaryExp: %s\n", operatorTypeToString(node->binaryExpr.operator->operator.opType));
            PRINT_CHILDREN(childPrefix, node->binaryExpr.leftExp, node->binaryExpr.rightExp);
            break;

        case NODE_UNARY_EXPR:
            printf("unaryExp: %s\n", operatorTypeToString(node->unaryExpr.operator->operator.opType));
            PRINT_CHILDREN(childPrefix, node->unaryExpr.exp);
            break;

        case NODE_CALL_EXPR:
        {
            printf("CallExpr\n");

            int total = node->callExpr.callee ? 1 : 0;
            for (int i = 0; i < node->callExpr.argCount; i++)
                if (node->callExpr.args[i]) total++;

            int seen = 0;
            if (node->callExpr.callee)
            {
                seen++;
                printNodeAt(node->callExpr.callee, childPrefix, seen == total, false);
            }
            for (int i = 0; i < node->callExpr.argCount; i++)
            {
                if (!node->callExpr.args[i]) continue;
                seen++;
                printNodeAt(node->callExpr.args[i], childPrefix, seen == total, false);
            }
            break;
        }

        case NODE_MEMBER_EXPR:
            printf("Member: %s%s\n", node->memberExpr.isArrow ? "->" : ".", node->memberExpr.member);
            PRINT_CHILDREN(childPrefix, node->memberExpr.object);
            break;

        case NODE_INDEX_EXPR:
            printf("Index\n");
            PRINT_CHILDREN(childPrefix, node->indexExpr.target, node->indexExpr.index);
            break;

        case NODE_TERNARY_EXPR:
            printf("Ternary: ?:\n");
            PRINT_CHILDREN(childPrefix, node->ternaryExpr.conditionExp,
                           node->ternaryExpr.ifTrueExp, node->ternaryExpr.ifFalseExp);
            break;

        case NODE_CAST:
            printf("Cast\n");
            PRINT_CHILDREN(childPrefix, node->cast.type, node->cast.exp);
            break;

        case NODE_EXPR_STMT:
            printf("ExprStmt\n");
            PRINT_CHILDREN(childPrefix, node->exprStmt.expr);
            break;

        case NODE_IF_STMT:
            printf("IfStmt\n");
            PRINT_CHILDREN(childPrefix, node->ifStmt.expr, node->ifStmt.logic, node->ifStmt.elseStmt);
            break;

        case NODE_ELSE_STMT:
            printf("ElseStmt\n");
            PRINT_CHILDREN(childPrefix, node->elseStmt.logic);
            break;

        case NODE_RETURN_STMT:
            printf("ReturnStmt\n");
            PRINT_CHILDREN(childPrefix, node->returnStmt.expr);
            break;

        case NODE_WHILE_STMT:
            printf("WhileStmt\n");
            PRINT_CHILDREN(childPrefix, node->singleArgLoop.expr, node->singleArgLoop.logic);
            break;

        case NODE_REPEAT_STMT:
            printf("RepeatStmt\n");
            PRINT_CHILDREN(childPrefix, node->singleArgLoop.expr, node->singleArgLoop.logic);
            break;

        case NODE_UNTIL_STMT:
            printf("UntilStmt\n");
            PRINT_CHILDREN(childPrefix, node->singleArgLoop.expr, node->singleArgLoop.logic);
            break;

        case NODE_BLOCK:
            printf("Block\n");
            printChildArray(node->block.statements, node->block.count, childPrefix);
            break;

        case NODE_FUNCTION_DECL:
        {
            printf("FunctionDecl: %s returns ", node->functionDecl.name);
            printTypeInline(node->functionDecl.returnType);
            printf("\n");

            int paramCount = node->functionDecl.paramCount;
            int hasBody = node->functionDecl.body ? 1 : 0;
            int total = (paramCount == 0 ? 1 : paramCount) + hasBody;
            int seen = 0;

            if (paramCount == 0)
            {
                seen++;
                printLeafLine(childPrefix, seen == total, "void");
            }
            else
            {
                for (int i = 0; i < paramCount; i++)
                {
                    seen++;
                    printNodeAt(node->functionDecl.params[i], childPrefix, seen == total, false);
                }
            }

            if (node->functionDecl.body)
            {
                seen++;
                printNodeAt(node->functionDecl.body, childPrefix, seen == total, false);
            }
            break;
        }

        case NODE_PROGRAM:
        {
            printf("Program\n");

            int total = 0;
            for (int i = 0; i < node->program.importCount; i++)
                if (node->program.imports[i]) total++;
            for (int i = 0; i < node->program.globalCount; i++)
                if (node->program.globals[i]) total++;
            for (int i = 0; i < node->program.functionCount; i++)
                if (node->program.functions[i]) total++;

            int seen = 0;
            for (int i = 0; i < node->program.importCount; i++)
            {
                if (!node->program.imports[i]) continue;
                seen++;
                printLeafLine(childPrefix, seen == total, "Import: %s", node->program.imports[i]);
            }
            for (int i = 0; i < node->program.globalCount; i++)
            {
                if (!node->program.globals[i]) continue;
                seen++;
                printNodeAt(node->program.globals[i], childPrefix, seen == total, false);
            }
            for (int i = 0; i < node->program.functionCount; i++)
            {
                if (!node->program.functions[i]) continue;
                seen++;
                printNodeAt(node->program.functions[i], childPrefix, seen == total, false);
            }
            break;
        }

        default:
            printf("Unknown node type: %d\n", node->type);
    }
}

void printNode(Node *node) //STUB implement all nodes
{
    printNodeAt(node, "", true, true);
}