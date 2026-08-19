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

Node *parseExpression(Parser*, int);

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

bool isLiteral(Token *t)
{
    if(t->type == TOKEN_CHAR || t->type == TOKEN_STRING || t->type == TOKEN_NUMBER) return true;
    else return false;
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
        Node *valueLiteral = malloc(sizeof(Node));
        valueLiteral->type = NODE_LITERAL;
        valueLiteral->literal.value = valueTok.text;
        param->varDecl.value = valueLiteral;
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

OperatorType lookupOperator(Token t)
{
    if (t.type != TOKEN_PUNCTUATION && t.type != TOKEN_KEYWORD && t.type != TOKEN_OPERATOR)
    return OPERATOR_NONE;

    if (!strcmp(t.text, ";") || !strcmp(t.text, "{"))  return OPERATOR_END_EXP;
    if (!strcmp(t.text, "+"))  return OPERATOR_ADD;
    if (!strcmp(t.text, "="))  return OPERATOR_ASSIGN;
    if (!strcmp(t.text, "*"))  return OPERATOR_MULTIPLY;
    if (!strcmp(t.text, "&&")) return OPERATOR_LOGICAL_AND;
    //STUB, add more operators

    return OPERATOR_NONE;
}

Node* nudUnaryPlus(Parser *p)
{

}

Node* nudPrimary(Parser *p) //build literal or var node from previous token
{
    Token tok = PREV(p); //checks token we just skipped

    Node *node = malloc(sizeof(Node));
    if (isLiteral(&tok))
    {
        node->type = NODE_LITERAL;
        node->literal.value = tok.text;
    }
    else // identifier
    {
        node->type = NODE_VAR;
        node->var.name = tok.text;
    }
    return node;
}

Node* ledBinaryLeft(Parser *p, Node *left) //build new left, starts on right of op, creates binary expression with left, operator, right
{
    Node *exp = malloc(sizeof(Node)); //allocate expression node

    Token opToken = PREV(p); //operator token is token just skipped
    OperatorType opType = lookupOperator(opToken); //get operator type of current operator
    Node *op = malloc(sizeof(Node));

    op->type = NODE_OPERATOR;
    op->operator.lbp = rules[opType].operator.lbp; //save for print
    int rbp = rules[opType].operator.lbp; //saved for printing

    exp->type = NODE_BINARY_EXPR;
    exp->binaryExpr.leftExp = left; //given left is left of binary exp
    exp->binaryExpr.operator = op; //set operator to current operator
    exp->binaryExpr.rightExp = parseExpression(p, rbp); //parse right of exp

    return exp; //return binary expression as new left
}

Node* ledBinaryRight(Parser *p, Node *left) //right assoc version
{
    Node *exp = malloc(sizeof(Node));

    Token opToken = PREV(p);
    OperatorType opType = lookupOperator(opToken);
    Node *op = malloc(sizeof(Node));

    op->type = NODE_OPERATOR;
    op->operator.lbp = rules[opType].operator.lbp; //save for print
    int rbp = rules[opType].operator.lbp - 1; 

    exp->type = NODE_BINARY_EXPR;
    exp->binaryExpr.leftExp = left; 
    exp->binaryExpr.operator = op; 
    exp->binaryExpr.rightExp = parseExpression(p, rbp); //recurse for right exp

    return exp; //return binary expression as new left
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

    rules[OPERATOR_LOGICAL_AND] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 50,
            .nud = NULL,
            .led = ledBinaryLeft
        }
    };

    rules[OPERATOR_ADD] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 120,
            .nud = nudUnaryPlus,
            .led = ledBinaryLeft
        }
    };

    rules[OPERATOR_MULTIPLY] = (Node){
        .type = NODE_OPERATOR,
        .operator = {
            .lbp = 130,
            .nud = NULL,
            .led = ledBinaryLeft
        }
    };
}


Node *parseExpression(Parser *p, int rbp) //parses the actual stream of expression tokens
{
    Token tok = CUR(p);
    OperatorType opType = lookupOperator(tok); // maps Token -> OperatorType

    Node *left = malloc(sizeof(Node));
    if (opType != OPERATOR_NONE && rules[opType].operator.nud != NULL) //consume operator
    {
        skip(p, "advancing past prefix/nud token");
        left = rules[opType].operator.nud(p);
    }
    else if (isLiteral(&tok) || IS_IDENTIFIER(p)) //consume literal/variable
    {
        skip(p, "advancing past literal/identifier");
        left = nudPrimary(p); 
    }
    else //unexpected token error
    {
        parserError("Unexpected token '%s' at position %d, expected start of expression", tok.text, p->pos);
    }

    while (1)
    {
        Token nextTok = CUR(p);
        OperatorType nextOp = lookupOperator(nextTok); //peek next token
        int lbp = (nextOp != OPERATOR_NONE) ? rules[nextOp].operator.lbp : 0; //lbp is 0 if next is identifier/literal

        if (lbp <= rbp) break;

        skip(p, "advancing past infix/led token");
        left = rules[nextOp].operator.led(p, left); //call led
    }

    return left;
}

Node *handleIfStatement(Parser *p)
{
    
}

Node *handleExpressionStatement(Parser *p)
{
    Node *expressionStatement = malloc(sizeof(Node));
    int startpos = p->pos; //cache starting position
    while(!IS_SEMICOLON(p))
    {
        skip(p, "expected semicolon before end of file!");
    }
    p->pos = startpos;
    expressionStatement->type = NODE_EXPR_STMT;
    expressionStatement->exprStmt.expr = parseExpression(p, 0);
    return expressionStatement;
}

Node *handleStatement(Parser *p) //starts on whatever is after ; or }
{
    Node *statement;
    //logic for determining kind of statement will go here
    //STUB, not all statements implemented.
    if(IS_KEYWORD(p, "if"))
    {
        statement = handleIfStatement(p);
    }
    else statement = handleExpressionStatement(p);
    return statement;
}

Node *handleCodeBlock(Parser *p) //starts on opening {
{ 
    Node *block = malloc(sizeof(Node));
    block->type = NODE_BLOCK;

    int statementCap = 8;
    Node **statements = malloc(sizeof(Node*) * statementCap);
    int statementCount = 0;
    consume(p, TOKEN_PUNCTUATION, "{", "Expected opening { after function arguments!");
    int scope = 1; //consume opening { and set scope to 1
    do
    {
        if (IS_OPEN_BRACE(p)) scope++;
        if (IS_CLOSE_BRACE(p)) scope--;
        if (scope == 0) break; //exit early if end of code block
        if (IS_CLOSE_BRACE(p)) continue; //do not process statement if empty
        if (statementCount >= statementCap) 
        {
            statementCap *= 2;
            statements = realloc(statements, sizeof(Node*) * statementCap); 
        }
        statements[statementCount] = handleStatement(p);
        statementCount++;
        while (!IS_SEMICOLON(p) && !IS_CLOSE_BRACE(p))
        {
            skip(p, "Expected ; or } before end of file");
        }
        if(IS_SEMICOLON(p)) consume(p, TOKEN_PUNCTUATION, ";", "Expected ; to terminate statement!");
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