#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/*
SonjC Parser
Written Aug 2026
Parses tokens emitted by the lexer into an Abstract Syntax Tree.
*/

void vparserError(const char *message, va_list args)
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

static const char *tokenTypeName(TokenType t)
{
    switch (t)
    {
        case TOKEN_IDENTIFIER:  return "identifier";
        case TOKEN_KEYWORD:     return "keyword";
        case TOKEN_PUNCTUATION: return "punctuation";
        case TOKEN_NUMBER:      return "number";
        case TOKEN_STRING:      return "string";
        case TOKEN_CHAR:        return "char";
        case TOKEN_EOF:         return "end of file";
        default:                return "token";
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

Node *handleParam(Parser *p, bool decleration)
{
    if (decleration)
    {
        Node *param = malloc(sizeof(Node));
        param->type = NODE_VAR_DECL;
        Token typeTok = consume(p, TOKEN_KEYWORD, NULL, "Expected a type keyword to start parameter declaration");
        param->varDecl.name = typeTok.text;
        Token valueTok = consume(p, TOKEN_IDENTIFIER, NULL, "Expected parameter name after type");
        param->varDecl.value = valueTok.text;
        return param;
    }
    else
    {
        Node *param = malloc(sizeof(Node));
        bool literal =
            CUR(p).type == TOKEN_NUMBER ||
            CUR(p).type == TOKEN_STRING ||
            CUR(p).type == TOKEN_CHAR ||
            IS_KEYWORD(p, "true") ||
            IS_KEYWORD(p, "false");

        if (literal)
        {
            param->type = NODE_LITERAL;
            param->literal.value = CUR(p).text;
        }
        else if (IS_IDENTIFIER(p))
        {
            param->type = NODE_VAR;
            param->var.name = CUR(p).text;
        }
        else parserError("Invalid argument type at position: %d", p->pos);
        return param;
    }
}

Node **handleArgList(Parser *p, bool decleration, int *outCount)
{
   int paramCap = 8;
    Node **params = malloc(sizeof(Node*) * paramCap);
    int paramCount = 0;

    skip(p, "Expected ( after name to begin argument list"); // step off name, onto (
    consume(p, TOKEN_PUNCTUATION, "(", "Expected ( to begin argument list");

    while (!IS_CLOSE_PAREN(p))
    {
        if (IS_VOID_KW(p))
        {
            paramCount = 0;
            break;
        }
        if (paramCount >= paramCap) { paramCap *= 2; params = realloc(params, sizeof(Node*) * paramCap); }
        params[paramCount] = handleParam(p, decleration);
        paramCount++;
        while (!IS_COMMA(p) && !IS_CLOSE_PAREN(p))
        {
            skip(p, "Expected , or closing ) before end of file");
        }
        if (IS_COMMA(p)) skip(p, "Expected another argument after , before end of file");
    }

    *outCount = paramCount;
    return params;
}

Node *handleExpression(Parser *p)
{
    bool isCallExpression =
        IS_IDENTIFIER(p) &&
        p->tokens[p->pos + 1].type == TOKEN_PUNCTUATION &&
        !strcmp(p->tokens[p->pos + 1].text, "(");

    if (isCallExpression)
    {
        Node *callExpression = malloc(sizeof(Node));
        callExpression->type = NODE_CALL_EXPR;
        callExpression->callExpr.name = CUR(p).text;
        int paramCount = 0;
        Node **params = handleArgList(p, false, &paramCount);
        callExpression->callExpr.argCount = paramCount;
        callExpression->callExpr.args = params;
        return callExpression;
    }
}

Node *handleExpressionStatement(Parser *p)
{
    Node *expressionStatement = malloc(sizeof(Node));
    expressionStatement->type = NODE_EXPR_STMT;
    expressionStatement->exprStmt.expr = handleExpression(p);
    return expressionStatement;
}

Node *handleStatement(Parser *p) //starts on whatever is after ; or }
{
    Node *statement;
    //logic for determining kind of statement will go here
    //STUB, hardcoded to an expression statement
    statement = handleExpressionStatement(p);
    return statement;
}

Node *handleCodeBlock(Parser *p) //starts on opening {
{ 
    Node *block = malloc(sizeof(Node));
    block->type = NODE_BLOCK;

    int statementCap = 8;
    Node **statements = malloc(sizeof(Node*) * statementCap);
    int statementCount = 0;
    int scope = 0;
    do
    {
        if (IS_OPEN_BRACE(p)) scope++;
        if (IS_CLOSE_BRACE(p)) scope--;
        if (scope == 0) break; //exit early if end of code block
        if (IS_CLOSE_BRACE(p)) continue; //do not process statement if empty
        if (statementCount >= statementCap) { statementCap *= 2; statements = realloc(statements, sizeof(Node*) * statementCap); }
        statements[statementCount] = handleStatement(p);
        statementCount++;
        while (!IS_SEMICOLON(p) && !IS_CLOSE_BRACE(p))
        {
            skip(p, "Expected ; or } before end of file");
        }
    } while (scope != 0);
    consume(p, TOKEN_PUNCTUATION, "}", "Expected } at end of code block");
    block->block.statements = statements;
    block->block.count = statementCount;
    return block;
}

char *handleImport(Parser *p)
{
    if (p->pos <= 0) parserError("Position for import identifier landed at or before 0");
    Token nameTok = consume(p, TOKEN_IDENTIFIER, NULL, "Expected import name");
    consume(p, TOKEN_PUNCTUATION, ";", "Expected ; after import name");
    return nameTok.text;
}

Node *handleGlobal(Parser *p) //STUB
{
    Node *global = malloc(sizeof(Node));
    global->type = NODE_VAR_DECL; // placeholder
    return global;
}

Node *handleFunction(Parser *p)
{
    if (p->pos <= 0) parserError("Position for function name landed at or before 0");

    Node *function = malloc(sizeof(Node));
    function->type = NODE_FUNCTION_DECL;
    function->functionDecl.name = CUR(p).text;
    if (p->tokens[p->pos - 1].type != TOKEN_KEYWORD) function->functionDecl.returnType = "void";
    else function->functionDecl.returnType = p->tokens[p->pos - 1].text;

    int paramCount;
    Node **params = handleArgList(p, true, &paramCount);
    function->functionDecl.paramCount = paramCount;
    function->functionDecl.params = params;

    skip(p, "Expected { after parameter list"); // step off closing )
    if (!IS_OPEN_BRACE(p)) parserError("Expected { to begin function body at position %d", p->pos);
    function->functionDecl.body = handleCodeBlock(p);
    return function;
}

Node *parse(Parser *p)
{
    int scope = 0;

    int importCount = 0;
    int functionCount = 0;
    int globalCount = 0;

    int importCap = 8, functionCap = 8, globalCap = 8;
    char **imports = malloc(sizeof(char*) * importCap);
    Node **functions = malloc(sizeof(Node*) * functionCap);
    Node **globals = malloc(sizeof(Node*) * globalCap);

    while (CUR(p).type != TOKEN_EOF)
    {
        bool isImport =
            scope == 0 &&
            p->pos > 0 &&
            p->tokens[p->pos - 1].type == TOKEN_KEYWORD &&
            IS_IDENTIFIER(p) &&
            !strcmp(p->tokens[p->pos - 1].text, "import");
        bool isFunction =
            scope == 0 &&
            p->pos > 0 &&
            IS_IDENTIFIER(p) &&
            p->tokens[p->pos + 1].type == TOKEN_PUNCTUATION &&
            !strcmp(p->tokens[p->pos + 1].text, "(");
        bool isGlobal =
            scope == 0 &&
            p->pos > 0 &&
            p->tokens[p->pos - 1].type == TOKEN_KEYWORD &&
            IS_IDENTIFIER(p) &&
            strcmp(p->tokens[p->pos - 1].text, "import");

        if (IS_OPEN_BRACE(p)) scope++;
        if (IS_CLOSE_BRACE(p)) scope--;

        bool handled = false;

        if (isImport)
        {
            if (importCount >= importCap) { importCap *= 2; imports = realloc(imports, sizeof(char*) * importCap); }
            imports[importCount] = handleImport(p);
            importCount++;
            handled = true;
        }
        if (isFunction)
        {
            if (functionCount >= functionCap) { functionCap *= 2; functions = realloc(functions, sizeof(Node*) * functionCap); }
            functions[functionCount] = handleFunction(p);
            functionCount++;
            handled = true;
        }
        if (isGlobal)
        {
            if (globalCount >= globalCap) { globalCap *= 2; globals = realloc(globals, sizeof(Node*) * globalCap); }
            globals[globalCount] = handleGlobal(p);
            globalCount++;
            handled = true;
        }

        if (!handled) p->pos++;
    }

    Node *programNode = malloc(sizeof(Node));
    *programNode = (Node){
        .type = NODE_PROGRAM,
        .program = {
            .imports = imports,
            .importCount = importCount,
            .functions = functions,
            .functionCount = functionCount,
            .globals = globals,
            .globalCount = globalCount,
        }
    };
    return programNode;
}