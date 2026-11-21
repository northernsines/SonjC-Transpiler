#include "print.h"
#include "parser.h"
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/*
SonjC Print
Written Sep 2026
Debug output helpers for token streams and the abstract syntax tree.
*/

#define TOKEN_TEXT_WIDTH_MAX 48

static void formatTokenText(const char* text, char* out, size_t outSize) {
    if (outSize == 0)
        return;
    if (!text)
        text = "";

    size_t oi = 0;
    bool truncated = false;

    for (size_t i = 0; text[i] != '\0'; i++) {
        char single[2] = {text[i], '\0'};
        const char* rep;

        switch (text[i]) {
            case '\n':
                rep = "\\n";
                break;
            case '\t':
                rep = "\\t";
                break;
            case '\r':
                rep = "\\r";
                break;
            default:
                rep = single;
                break;
        }

        size_t rl = strlen(rep);
        if (oi + rl + 1 > outSize) {
            truncated = true;
            break;
        }

        memcpy(out + oi, rep, rl);
        oi += rl;
    }

    if (truncated && outSize >= 4) {
        size_t keep = outSize - 4;
        if (oi > keep)
            oi = keep;
        memcpy(out + oi, "...", 3);
        oi += 3;
    }

    out[oi] = '\0';
}

static void printHorizontalRule(const char* left, const char* mid, const char* right,
                                int idxWidth, int typeWidth, int textWidth) {
    printf("%s", left);
    for (int i = 0; i < idxWidth + 2; i++)
        printf("─");
    printf("%s", mid);
    for (int i = 0; i < typeWidth + 2; i++)
        printf("─");
    printf("%s", mid);
    for (int i = 0; i < textWidth + 2; i++)
        printf("─");
    printf("%s\n", right);
}

void printTokenStream(Token* tokens, int count) {
    int idx_width = 3;
    if (count > 0) {
        int digits = snprintf(NULL, 0, "%d", count - 1);
        if (digits > idx_width)
            idx_width = digits;
    }

    int type_width = 4;
    int text_width = 4;

    for (int i = 0; i < count; i++) {
        char display[TOKEN_TEXT_WIDTH_MAX + 1];
        formatTokenText(tokens[i].text, display, sizeof(display));

        int len = (int)strlen(display);
        if (len > text_width)
            text_width = len;
    }

    printHorizontalRule("┌", "┬", "┐", idx_width, type_width, text_width);
    printf("│ %-*s │ %-*s │ %-*s │\n", idx_width, "idx", type_width, "type", text_width, "text");
    printHorizontalRule("├", "┼", "┤", idx_width, type_width, text_width);

    for (int i = 0; i < count; i++) {
        char display[TOKEN_TEXT_WIDTH_MAX + 1];
        formatTokenText(tokens[i].text, display, sizeof(display));

        printf("│ %*d │ %-*s │ %-*s │\n",
               idx_width, i,
               type_width, tokentypeToString(tokens[i].type),
               text_width, display);
    }

    printHorizontalRule("└", "┴", "┘", idx_width, type_width, text_width);
    fflush(stdout);
}

const char* tokentypeToString(TokenType type) {
    return type == TOKEN_KEYWORD ? "KEYW" : type == TOKEN_IDENTIFIER ? "IDEN"
                                        : type == TOKEN_PUNCTUATION  ? "PUNC"
                                        : type == TOKEN_OPERATOR     ? "OPER"
                                        : type == TOKEN_STRING       ? "STRN"
                                        : type == TOKEN_INTEGER      ? "INTG"
                                        : type == TOKEN_FLOAT        ? "FLOT"
                                        : type == TOKEN_FIXED        ? "FIXD"
                                        : type == TOKEN_NONE         ? "NONE"
                                        : type == TOKEN_EOF          ? "EOFL"
                                        : type == TOKEN_CHAR         ? "CHAR"
                                                                     : "UKWN";
}

const char* operatorTypeToString(OperatorType op) {
    switch (op) {
        case OPERATOR_END_EXP:
            return "endExp";
        case OPERATOR_NONE:
            return "none";
        case OPERATOR_COMMA:
            return ",";
        case OPERATOR_ASSIGN:
            return "=";
        case OPERATOR_ASSIGN_ADD:
            return "+=";
        case OPERATOR_ASSIGN_SUBTRACT:
            return "-=";
        case OPERATOR_ASSIGN_MULTIPLY:
            return "*=";
        case OPERATOR_ASSIGN_DIVIDE:
            return "/=";
        case OPERATOR_ASSIGN_MOD:
            return "%=";
        case OPERATOR_BITWISE_ASSIGN_AND:
            return "&=";
        case OPERATOR_BITWISE_ASSIGN_OR:
            return "|=";
        case OPERATOR_BITWISE_ASSIGN_XOR:
            return "^=";
        case OPERATOR_BITWISE_ASSIGN_LEFTSHIFT:
            return "<<=";
        case OPERATOR_BITWISE_ASSIGN_RIGHTSHIFT:
            return ">>=";
        case OPERATOR_TERNARY:
            return "?";
        case OPERATOR_LOGICAL_OR:
            return "||";
        case OPERATOR_LOGICAL_AND:
            return "&&";
        case OPERATOR_BITWISE_AND:
            return "&";
        case OPERATOR_BITWISE_OR:
            return "|";
        case OPERATOR_BITWISE_XOR:
            return "^";
        case OPERATOR_IS_EQUAL:
            return "==";
        case OPERATOR_IS_NOT_EQUAL:
            return "!=";
        case OPERATOR_GREATER_THAN:
            return ">";
        case OPERATOR_LESS_THAN:
            return "<";
        case OPERATOR_LESS_THAN_OREQ:
            return "<=";
        case OPERATOR_GREATER_THAN_OREQ:
            return ">=";
        case OPERATOR_SHIFT_LEFT:
            return "<<";
        case OPERATOR_SHIFT_RIGHT:
            return ">>";
        case OPERATOR_ADD:
            return "+";
        case OPERATOR_SUBTRACT:
            return "-";
        case OPERATOR_MULTIPLY:
            return "*";
        case OPERATOR_DIVIDE:
            return "/";
        case OPERATOR_MOD:
            return "%";
        case OPERATOR_CAST:
            return "cast";
        case OPERATOR_LOGICAL_NOT:
            return "!";
        case OPERATOR_BITWISE_NOT:
            return "~";
        case OPERATOR_SIZEOF:
            return "sizeof";
        case OPERATOR_INCREMENT:
            return "++";
        case OPERATOR_DECREMENT:
            return "--";
        case OPERATOR_BRACKET:
            return "[]";
        case OPERATOR_DOT:
            return ".";
        case OPERATOR_ARROW:
            return "->";
            //STUB add more operators
    }
    return "unknown";
}

static const char* primitiveTypeToString(PrimitiveType type) {
    switch (type) {
        case PRIMITIVE_INT:
            return "int";
        case PRIMITIVE_CHAR:
            return "char";
        case PRIMITIVE_FLOAT:
            return "float";
        case PRIMITIVE_VOID:
            return "void";
        case PRIMITIVE_FIXED:
            return "fixed";
        case PRIMITIVE_BOOL:
            return "bool";
        case PRIMITIVE_COUNT:
            break;
    }
    return "unknown";
}

static void printTypeInline(Node* type);

static void printQualifierSuffix(Node* qualifier) {
    if (!qualifier || qualifier->type != NODE_TYPE_QUALIFIER)
        return;

    switch (qualifier->type_qualifier.kind) {
        case QUAL_POINTER:
            printf("*");
            break;
        case QUAL_CONST:
            printf(" const");
            break;
        case QUAL_VOLATILE:
            printf(" volatile");
            break;
        case QUAL_UNSIGNED:
            printf(" unsigned");
            break;
        case QUAL_RAW:
            printf(" raw");
            break;
        case QUAL_LONG:
            printf(" long");
            break;
        case QUAL_SHORT:
            printf(" short");
            break;
        case QUAL_ARRAY:
            printf("[%d]", qualifier->type_qualifier.array_size);
            break;
        case QUAL_FUNCTION:
            printf("(");
            for (int i = 0; i < qualifier->type_qualifier.func_qual.param_count; i++) {
                if (i > 0)
                    printf(", ");
                printTypeInline(qualifier->type_qualifier.func_qual.params[i]);
            }
            printf(")");
            break;
        default:
            printf("?");
            break;
    }
}

static void printTypeInline(Node* type) {
    if (!type)
        return;

    switch (type->type) {
        case NODE_TYPE:
            printTypeInline(type->type_node.inner_type);
            printQualifierSuffix(type->type_node.qualifier);
            break;
        case NODE_PRIMITIVE:
            printf("%s", primitiveTypeToString(type->primitive_type.type));
            break;
        case NODE_TYPEREF:
            printf("%s", type->type_ref.name);
            break;
        default:
            printf("<not a type>");
            break;
    }
}

#define AST_PREFIX_MAX 1024

static void printNodeAt(Node* node, const char* prefix, bool isLast, bool isRoot);

static void printLeafLine(const char* prefix, bool isLast, const char* fmt, ...) {
    va_list args;

    printf("%s%s ", prefix, isLast ? "└──" : "├──");
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

static void printChildArray(Node** children, int count, const char* prefix) {
    int total = 0;
    for (int i = 0; i < count; i++)
        if (children[i])
            total++;

    int seen = 0;
    for (int i = 0; i < count; i++) {
        if (!children[i])
            continue;
        seen++;
        printNodeAt(children[i], prefix, seen == total, false);
    }
}

#define PRINT_CHILDREN(prefix, ...)                                                 \
    do {                                                                            \
        Node* _children[] = {__VA_ARGS__};                                          \
        printChildArray(_children, (int)(sizeof(_children) / sizeof(_children[0])), \
                        (prefix));                                                  \
    } while (0)

static void printNodeAt(Node* node, const char* prefix, bool isLast, bool isRoot) {
    if (!node)
        return;

    if (!isRoot)
        printf("%s%s ", prefix, isLast ? "└──" : "├──");

    char child_prefix[AST_PREFIX_MAX];
    if (isRoot) {
        child_prefix[0] = '\0';
    } else {
        snprintf(child_prefix, sizeof(child_prefix), "%s%s", prefix, isLast ? "    " : "│   ");
    }

    switch (node->type) {
        case NODE_INT_LITERAL:
            printf("Int: %d\n", node->int_literal.value);
            break;

        case NODE_FLOAT_LITERAL:
            printf("Float: %f\n", node->float_literal.value);
            break;

        case NODE_FIXED_LITERAL:
            printf("Fixed: %" PRIu32 "\n", node->fixed_literal.value);
            break;

        case NODE_CHAR_LITERAL:
            printf("Char: %c \n", node->char_literal.value);
            break;

        case NODE_STRING_LITERAL:
            printf("String: %s\n", node->string_literal.value);
            break;

        case NODE_BOOL_LITERAL:
            printf("Bool: %s\n", node->bool_literal.value ? "true" : "false");
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
            printf("Typedef: %s =", node->type_decl.alias_name);
            if (node->type_decl.underlying_instance &&
                node->type_decl.underlying_instance->type == NODE_OBJECT_DECL) {
                printf("\n");
                printNodeAt(node->type_decl.underlying_instance, child_prefix, true, false);
            } else {
                printf(" ");
                printTypeInline(node->type_decl.underlying_instance);
                printf("\n");
            }
            break;

        case NODE_OBJECT_DECL:
            printf("ObjectDecl: %s\n", node->object_decl.name ? node->object_decl.name : "<anonymous>");
            printChildArray(node->object_decl.members, node->object_decl.member_count, child_prefix);
            break;

        case NODE_VAR:
            printf("Id: %s\n", node->var.name);
            break;

        case NODE_VAR_DECL:
            printf("VarDecl\n");
            PRINT_CHILDREN(child_prefix, node->var_decl.type, node->var_decl.var, node->var_decl.value_expr);
            break;

        case NODE_BINARY_EXPR:
            printf("binaryExp: %s\n", operatorTypeToString(node->binary_expr.operator->operator.op_type));
            PRINT_CHILDREN(child_prefix, node->binary_expr.left_exp, node->binary_expr.right_exp);
            break;

        case NODE_UNARY_EXPR:
            printf("unaryExp: %s\n", operatorTypeToString(node->unary_expr.operator->operator.op_type));
            PRINT_CHILDREN(child_prefix, node->unary_expr.exp);
            break;

        case NODE_CALL_EXPR: {
            printf("CallExpr\n");

            int total = node->call_expr.callee ? 1 : 0;
            for (int i = 0; i < node->call_expr.arg_count; i++)
                if (node->call_expr.args[i])
                    total++;

            int seen = 0;
            if (node->call_expr.callee) {
                seen++;
                printNodeAt(node->call_expr.callee, child_prefix, seen == total, false);
            }
            for (int i = 0; i < node->call_expr.arg_count; i++) {
                if (!node->call_expr.args[i])
                    continue;
                seen++;
                printNodeAt(node->call_expr.args[i], child_prefix, seen == total, false);
            }
            break;
        }

        case NODE_MEMBER_EXPR:
            printf("Member: %s%s\n", node->member_expr.is_arrow ? "->" : ".", node->member_expr.member);
            PRINT_CHILDREN(child_prefix, node->member_expr.object);
            break;

        case NODE_INDEX_EXPR:
            printf("Index\n");
            PRINT_CHILDREN(child_prefix, node->index_expr.target, node->index_expr.index);
            break;

        case NODE_TERNARY_EXPR:
            printf("Ternary: ?:\n");
            PRINT_CHILDREN(child_prefix, node->ternary_expr.condition_exp,
                           node->ternary_expr.if_true_exp, node->ternary_expr.if_false_exp);
            break;

        case NODE_CAST:
            printf("Cast\n");
            PRINT_CHILDREN(child_prefix, node->cast.type, node->cast.exp);
            break;

        case NODE_EXPR_STMT:
            printf("ExprStmt\n");
            PRINT_CHILDREN(child_prefix, node->expr_stmt.expr);
            break;

        case NODE_IF_STMT:
            printf("IfStmt\n");
            PRINT_CHILDREN(child_prefix, node->if_stmt.expr, node->if_stmt.logic, node->if_stmt.else_stmt);
            break;

        case NODE_WHEN_STMT:
            printf("WhenStmt\n");
            PRINT_CHILDREN(child_prefix, node->when_stmt.expr, node->when_stmt.logic, node->when_stmt.else_stmt);
            break;

        case NODE_ELSE_STMT:
            printf("ElseStmt\n");
            PRINT_CHILDREN(child_prefix, node->else_stmt.logic);
            break;

        case NODE_RETURN_STMT:
            printf("ReturnStmt\n");
            PRINT_CHILDREN(child_prefix, node->return_stmt.expr);
            break;

        case NODE_WHILE_STMT:
            printf("WhileStmt\n");
            PRINT_CHILDREN(child_prefix, node->single_arg_loop.expr, node->single_arg_loop.logic);
            break;

        case NODE_REPEAT_STMT:
            printf("RepeatStmt\n");
            PRINT_CHILDREN(child_prefix, node->single_arg_loop.expr, node->single_arg_loop.logic);
            break;

        case NODE_UNTIL_STMT:
            printf("UntilStmt\n");
            PRINT_CHILDREN(child_prefix, node->single_arg_loop.expr, node->single_arg_loop.logic);
            break;

        case NODE_DO_WHILE_STMT:
            printf("DoWhileStmt\n");
            PRINT_CHILDREN(child_prefix, node->do_loop.logic, node->do_loop.expr);
            break;

        case NODE_DO_UNTIL_STMT:
            printf("DoUntilStmt\n");
            PRINT_CHILDREN(child_prefix, node->do_loop.logic, node->do_loop.expr);
            break;

        case NODE_ITERATE_STMT:
            printf("IterateStmt\n");
            PRINT_CHILDREN(child_prefix, node->iterate_loop.source, node->iterate_loop.item, node->iterate_loop.logic);
            break;

        case NODE_BLOCK:
            printf("Block\n");
            printChildArray(node->block.statements, node->block.count, child_prefix);
            break;

        case NODE_FUNCTION_DECL: {
            printf("FunctionDecl: %s returns ", node->function_decl.name);
            printTypeInline(node->function_decl.return_type);
            printf("\n");

            int param_count = node->function_decl.param_count;
            int has_body = node->function_decl.body ? 1 : 0;
            int total = (param_count == 0 ? 1 : param_count) + has_body;
            int seen = 0;

            if (param_count == 0) {
                seen++;
                printLeafLine(child_prefix, seen == total, "void");
            } else {
                for (int i = 0; i < param_count; i++) {
                    seen++;
                    printNodeAt(node->function_decl.params[i], child_prefix, seen == total, false);
                }
            }

            if (node->function_decl.body) {
                seen++;
                printNodeAt(node->function_decl.body, child_prefix, seen == total, false);
            }
            break;
        }

        case NODE_PROGRAM: {
            printf("Program\n");

            int total = 0;
            for (int i = 0; i < node->program.import_count; i++)
                if (node->program.imports[i])
                    total++;
            for (int i = 0; i < node->program.global_count; i++)
                if (node->program.globals[i])
                    total++;
            for (int i = 0; i < node->program.function_count; i++)
                if (node->program.functions[i])
                    total++;

            int seen = 0;
            for (int i = 0; i < node->program.import_count; i++) {
                if (!node->program.imports[i])
                    continue;
                seen++;
                printLeafLine(child_prefix, seen == total, "Import: %s", node->program.imports[i]);
            }
            for (int i = 0; i < node->program.global_count; i++) {
                if (!node->program.globals[i])
                    continue;
                seen++;
                printNodeAt(node->program.globals[i], child_prefix, seen == total, false);
            }
            for (int i = 0; i < node->program.function_count; i++) {
                if (!node->program.functions[i])
                    continue;
                seen++;
                printNodeAt(node->program.functions[i], child_prefix, seen == total, false);
            }
            break;
        }

        default:
            printf("Unknown node type: %d\n", node->type);
    }
}

//STUB implement all nodes
void printNode(Node* node) {
    printNodeAt(node, "", true, true);
}