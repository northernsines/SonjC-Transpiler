#ifndef PARSER_H
#define PARSER_H
#include "lexer.h"
#include <stdint.h>

/*
SonjC Parser Header File
Written Aug 2026
Defines the various types of node on the Abstract Syntax Tree, as well as other data types and macros.
*/

typedef enum {
    NODE_PROGRAM,
    NODE_FUNCTION_DECL,
    NODE_VAR_DECL,
    NODE_BLOCK,
    NODE_EXPR_STMT,
    NODE_CALL_EXPR,
    NODE_UNARY_EXPR,
    NODE_BINARY_EXPR,
    NODE_TERNARY_EXPR,
    NODE_VAR,
    NODE_INT_LITERAL,
    NODE_FLOAT_LITERAL,
    NODE_FIXED_LITERAL,
    NODE_CHAR_LITERAL,
    NODE_STRING_LITERAL,
    NODE_OPERATOR,
    NODE_IF_STMT,
    NODE_SWITCH_STMT,
    NODE_WHILE_STMT,
    NODE_DO_WHILE_STMT,
    NODE_FOR_STMT,
    NODE_ITERATE_STMT,
    NODE_RETURN_STMT,
    NODE_BREAK_STMT,
    NODE_CONTINUE_STMT,
    NODE_GOTO_STMT,
    NODE_LABEL_STMT,
    NODE_INSTANCE_DECL,
    NODE_BOOL_LITERAL
} NodeType;

typedef enum {
    OPERATOR_END_EXP,
    OPERATOR_NONE,
    //comma
    OPERATOR_COMMA,
    //assignment
    OPERATOR_ASSIGN,
    //ternary
    //logical AND and OR
    OPERATOR_LOGICAL_OR,
    OPERATOR_LOGICAL_AND,
    //bitwise AND OR and XOR
    OPERATOR_BITWISE_AND,
    OPERATOR_BITWISE_OR,
    OPERATOR_BITWISE_XOR,
    //equality
    OPERATOR_IS_EQUAL,
    OPERATOR_IS_NOT_EQUAL,
    //relational
    //shift
    //additive
    OPERATOR_ADD,
    OPERATOR_SUBTRACT,
    //multiplicative
    OPERATOR_MULTIPLY,
    OPERATOR_DIVIDE,
    //cast
    OPERATOR_CAST,
    //unary
    OPERATOR_LOGICAL_NOT,
    //postfix
    OPERATOR_POSTFIX_INCREMENT,
    OPERATOR_POSTFIX_DECREMENT
} OperatorType; //grouped by lbp

typedef struct {
    Token *tokens;
    int tokenCount;
    int pos;
} Parser;

typedef struct Node{
    NodeType type;

    union {
        struct { // top level program
            char **imports;
            int importCount;
            struct Node **functions;
            int functionCount;
            struct Node **globals;
            int globalCount;
        } program;

        struct { // function declaration
            char *returnType;
            char *name;
            struct Node **params;
            int paramCount;
            struct Node *body;
        } functionDecl;

        struct { // variable declaration
            char *name;
            struct Node *value;
        } varDecl;

        struct { // typedef aliasing
            char *aliasName;
            struct Node *underlyingInstance;
            char *underlyingTypeName;
        } typeDecl;

        struct { // instance declaration
            char *name;
            struct Node **fields;
            int fieldCount;
            struct Node **methods;
            int methodCount;
        } instanceDecl;

        struct { // block
            struct Node **statements;
            int count;
        } block;

        struct { // expression statement
            struct Node *expr;
        } exprStmt;

        struct { // call expression
            char *name;
            struct Node **args;
            int argCount;
        } callExpr;

        struct { // variable reference
            char *name;
        } var;

        struct { // int literal
            int value;
        } intLiteral;

        struct { // float literal
            float value;
        } floatLiteral;

        struct { // fixed literal
            __uint64_t value;
        } fixedLiteral;

        struct { // char literal
            char value;
        } charLiteral;

        struct { // string literal
            char* value;
        } stringLiteral;

        struct { // bool literal
            bool value;
        } boolLiteral;        

        struct { // operator
            int lbp;
            struct Node *(*nud)(Parser *p);
            struct Node *(*led)(Parser *p, struct Node *left);
        } operator;

        struct { // unary expression
            struct Node *operator;
            struct Node *exp;
        } unaryExpr;

        struct { // binary expression
            struct Node *operator;
            struct Node *leftExp;
            struct Node *rightExp;
        } binaryExpr;

        struct { // ternary expression
            struct Node *operator;
            struct Node *conditionExp;
            struct Node *ifTrueExp;
            struct Node *ifFalseExp;
        } ternaryExpr;
        
    };
} Node;

// current-token shorthand
#define PREV(p)              ((p)->tokens[(p)->pos-1])
#define CUR(p)               ((p)->tokens[(p)->pos])
#define NEXT(p)              ((p)->tokens[(p)->pos+1])

// common token-check macros
#define IS_PUNCT(p, s)       (CUR(p).type == TOKEN_PUNCTUATION && !strcmp(CUR(p).text, (s)))
#define IS_KEYWORD(p, s)     (CUR(p).type == TOKEN_KEYWORD && !strcmp(CUR(p).text, (s)))
#define IS_IDENTIFIER(p)     (CUR(p).type == TOKEN_IDENTIFIER)
#define IS_LITERAL(p) ( \
    CUR(p).type == TOKEN_INTEGER || \
    CUR(p).type == TOKEN_STRING  || \
    CUR(p).type == TOKEN_CHAR    || \
    CUR(p).type == TOKEN_FLOAT   || \
    CUR(p).type == TOKEN_FIXED   || \
    (CUR(p).type == TOKEN_KEYWORD && (!strcmp(CUR(p).text, "true") || !strcmp(CUR(p).text, "false"))) \
)

#define PREV_PUNCT(p, s)     (PREV(p).type == TOKEN_PUNCTUATION && !strcmp(CUR(p).text, (s)))
#define PREV_KEYWORD(p, s)   (PREV(p).type == TOKEN_KEYWORD && !strcmp(CUR(p).text, (s)))
#define PREV_IDENTIFIER(p)   (PREV(p).type == TOKEN_IDENTIFIER)
#define PREV_LITERAL(p) ( \
    PREV(p).type == TOKEN_INTEGER || \
    PREV(p).type == TOKEN_STRING  || \
    PREV(p).type == TOKEN_CHAR    || \
    PREV(p).type == TOKEN_FLOAT   || \
    PREV(p).type == TOKEN_FIXED   || \
    (PREV(p).type == TOKEN_KEYWORD && (!strcmp(PREV(p).text, "true") || !strcmp(PREV(p).text, "false"))) \
)

#define NEXT_PUNCT(p, s)     (PREV(p).type == TOKEN_PUNCTUATION && !strcmp(CUR(p).text, (s)))
#define NEXT_KEYWORD(p, s)   (PREV(p).type == TOKEN_KEYWORD && !strcmp(CUR(p).text, (s)))
#define NEXT_IDENTIFIER(p)   (PREV(p).type == TOKEN_IDENTIFIER)

#define IS_OPEN_PAREN(p)     IS_PUNCT(p, "(")
#define IS_CLOSE_PAREN(p)    IS_PUNCT(p, ")")
#define IS_OPEN_BRACE(p)     IS_PUNCT(p, "{")
#define IS_CLOSE_BRACE(p)    IS_PUNCT(p, "}")
#define IS_COMMA(p)          IS_PUNCT(p, ",")
#define IS_SEMICOLON(p)      IS_PUNCT(p, ";")
#define IS_VOID_KW(p)        IS_KEYWORD(p, "void")


//neccessary function declerations
Token consume(Parser *p, TokenType expectedType, const char *expectedText, const char *context);
void skip(Parser *p, const char *context);
Node *parse(Parser *p);

//operator rule table
void initRules(void);
extern Node rules[17];
#endif