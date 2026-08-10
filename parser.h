#ifndef PARSER_H
#define PARSER_H
#include "lexer.h"

/*
SonjC Parser Header File
Written Aug 2026
Defines the various types of node on the Abstract Syntax Tree.
*/

typedef enum { //define different kinds of nodes
    NODE_PROGRAM,
    NODE_FUNCTION_DECL,
    NODE_VAR_DECL,
    NODE_BLOCK,
    NODE_EXPR_STMT,
    NODE_CALL_EXPR,
    NODE_BINARY_EXPR,
    NODE_UNARY_EXPR,
    NODE_VAR,
    NODE_LITERAL,
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
} NodeType;

typedef struct Node {
    NodeType type;   // which kind of node 

    union {
        struct { //top level program
            char **imports; //module imports
            int importCount;
            struct Node **functions; //functions
            int functionCount;
            struct Node **globals; //global variables
            int globalCount;
        } program;

        struct { //function declaration
            char *returnType;
            char *name;
            struct Node **params;
            int paramCount;
            struct Node *body;   // pointer to a NODE_BLOCK
        } functionDecl;

        struct { //variable declaration
            char *name;
            char *value;
        } varDecl;

        struct { //typedef aliasing
            char *aliasName;
            struct Node *underlyingInstance; // set when aliasing an inline struct/union/enum/object
            char *underlyingTypeName;        // set when aliasing a plain existing type (e.g. "int", "MyStruct")
        } typeDecl;

        struct { //instance decleration 
            char *name;
            struct Node **fields;
            int fieldCount;
            struct Node **methods;
            int methodCount;
        } instanceDecl;

        struct { //block of statements enclosed by {}
            struct Node **statements; 
            int count;
        } block;

        struct { //expression statement (ie procedure invocations or assignment)
            struct Node *expr;
        } exprStmt;

        struct { //call expression, like calling a function to return a value
            char *name;
            struct Node **args;
            int argCount;
        } callExpr;

        struct { //variable reference
            char *name;
        } var;

        struct { //variable reference
            char *value;
        }literal;

        // ... one struct per NodeType (STUB currently)
    };
} Node;

typedef struct {
    Token *tokens;
    int tokenCount;
    int pos;
} Parser;

// current-token shorthand
#define CUR(p) ((p)->tokens[(p)->pos])

// common token-check macros
#define IS_PUNCT(p, s)      (CUR(p).type == TOKEN_PUNCTUATION && !strcmp(CUR(p).text, (s)))
#define IS_KEYWORD(p, s)    (CUR(p).type == TOKEN_KEYWORD && !strcmp(CUR(p).text, (s)))
#define IS_IDENTIFIER(p)    (CUR(p).type == TOKEN_IDENTIFIER)

#define IS_OPEN_PAREN(p)    IS_PUNCT(p, "(")
#define IS_CLOSE_PAREN(p)   IS_PUNCT(p, ")")
#define IS_OPEN_BRACE(p)    IS_PUNCT(p, "{")
#define IS_CLOSE_BRACE(p)   IS_PUNCT(p, "}")
#define IS_COMMA(p)         IS_PUNCT(p, ",")
#define IS_SEMICOLON(p)     IS_PUNCT(p, ";")
#define IS_VOID_KW(p)       IS_KEYWORD(p, "void")

Token consume(Parser *p, TokenType expectedType, const char *expectedText, const char *context);
void skip(Parser *p, const char *context);
Node *parse(Parser *p);

#endif