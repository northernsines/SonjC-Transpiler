#include "parser.h"
#include <stdlib.h>
#include <string.h>

/*
SonjC Parser
Written Aug 2026
Parses tokens emitted by the lexer into an Abstract Syntax Tree.
*/

Node rules[60];
Node *parseExpression(Parser*, int);
Node *handleStatement(Parser*);
Node *handleCodeBlock(Parser*);
Node *parseType(Parser*);
Node *handleTypedef(Parser*);
Node *handleGlobal(Parser*);

Node *handleLiteral(Parser *p)
{
    Token t = CUR(p);
    Node *literal = malloc(sizeof(Node));
    switch(t.type) //map tokentype to node types for literals
    {
        case TOKEN_INTEGER: literal->type = NODE_INT_LITERAL;    break;
        case TOKEN_FLOAT:   literal->type = NODE_FLOAT_LITERAL;  break;
        case TOKEN_FIXED:   literal->type = NODE_FIXED_LITERAL;  break;
        case TOKEN_CHAR:    literal->type = NODE_CHAR_LITERAL;   break;
        case TOKEN_STRING:  literal->type = NODE_STRING_LITERAL; break;
        case TOKEN_KEYWORD: literal->type = NODE_BOOL_LITERAL;   break;
        default: parserError("Expected literal in literal slot at position %d", p->pos);
    }
    char* str = malloc(strlen(t.text) + 1);
    if (str == NULL) parserError("malloc for string literal allocation returned null");
    strcpy(str, t.text);
    int strSize = strlen(str);
    if(literal->type == NODE_INT_LITERAL) //convert values to proper types
    {
        literal->intLiteral.value = atoi(str);
    }
    else if(literal->type == NODE_FLOAT_LITERAL)
    {
        literal->floatLiteral.value = atof(str);
    }
    else if(literal->type == NODE_FIXED_LITERAL) // Q32.32 fixed point algorithm
    {
        int i;
        char *intPartStr = malloc(strlen(str) + 1);
        char *fracPartStr = malloc(strlen(str) + 1);
        for(i = 0; i < strSize; i++) //iterate until detect . build left string
        {
            char ch = str[i];
            bool isDot = ch == '.';
            if(isDot) break;
            else
            {
                intPartStr[i] = ch;
            }
        }
        intPartStr[i] = '\0';
        i++; //skip dot
        int ioffset = i;
        for(i = ioffset; i < strSize; i++) //iterate until null term, build right string
        {
            char ch = str[i];
            bool isNull = ch == '\0';
            if(isNull) break;
            else
            {
                fracPartStr[i-ioffset] = ch;
            }
        }
        fracPartStr[i-ioffset] = '\0';
        uint64_t intPart = strtoull(intPartStr, NULL, 10); 
        uint64_t fracDigits = strtoull(fracPartStr, NULL, 10);
        size_t fracLen = strlen(fracPartStr);

        __uint128_t scaled = (__uint128_t)fracDigits << 32; 
        uint64_t fracPart = (uint64_t)(scaled / pow10(fracLen)); // pow10 = your own int pow

        uint64_t underlying = (intPart << 32) | fracPart; 
        literal->fixedLiteral.value = underlying;
    }
    else if(literal->type == NODE_CHAR_LITERAL)
    {
        literal->charLiteral.value = *str;
    }
    else if(literal->type == NODE_STRING_LITERAL)
    {
       literal->stringLiteral.value = str;
    }
    else if(literal->type == NODE_BOOL_LITERAL)
    {
        if(!strcmp(str, "true")) literal->boolLiteral.value = true;
        else literal->boolLiteral.value = false;
    }
    skip(p, "advancing past literal");
    return literal;
}

static bool isTypeQualifierStart(Parser *p)
{
    return IS_OPEN_PAREN(p) || IS_OPEN_BRACKET(p) ||
           IS_OP(p, "*") ||
           IS_KEYWORD(p, "const") || IS_KEYWORD(p, "volatile");
}

static Node *makeSimpleQualifier(SimpleQualifier simpleQual, QualifierKind kind)
{
    Node *qualifier = malloc(sizeof(Node));
    qualifier->type = NODE_TYPE_QUALIFIER;
    qualifier->typeQualifier.kind = kind;
    qualifier->typeQualifier.simpleQual = simpleQual;
    return qualifier;
}

static Node *handleArrayQualifier(Parser *p)
{
    Node *qualifier = malloc(sizeof(Node));
    qualifier->type = NODE_TYPE_QUALIFIER;
    qualifier->typeQualifier.kind = QUAL_ARRAY;

    consume(p, TOKEN_PUNCTUATION, "[", "Expected [ to begin array type");
    Token sizeTok = consume(p, TOKEN_INTEGER, NULL, "Expected integer array size");
    qualifier->typeQualifier.arraySize = atoi(sizeTok.text);
    consume(p, TOKEN_PUNCTUATION, "]", "Expected ] after array size");

    return qualifier;
}

static Node *handleFuncQualifier(Parser *p)
{
    Node *qualifier = malloc(sizeof(Node));
    qualifier->type = NODE_TYPE_QUALIFIER;
    qualifier->typeQualifier.kind = QUAL_FUNCTION;

    int paramCap = 8;
    int paramCount = 0;
    Node **params = malloc(sizeof(Node*) * paramCap);

    consume(p, TOKEN_PUNCTUATION, "(", "Expected ( to begin function type");

    if (IS_CLOSE_PAREN(p))
    {
        consume(p, TOKEN_PUNCTUATION, ")", "Expected ) to close function type");
    }
    else if (IS_VOID_KW(p) && NEXT_PUNCT(p, ")"))
    {
        skip(p, "advancing past void in function type");
        consume(p, TOKEN_PUNCTUATION, ")", "Expected ) to close function type");
    }
    else
    {
        while (1)
        {
            if (paramCount >= paramCap)
            {
                paramCap *= 2;
                params = realloc(params, sizeof(Node*) * paramCap);
            }

            params[paramCount++] = parseType(p);

            if (IS_COMMA(p))
            {
                skip(p, "advancing past , in function type");
                continue;
            }
            break;
        }

        consume(p, TOKEN_PUNCTUATION, ")", "Expected ) to close function type");
    }

    qualifier->typeQualifier.funcQual.params = params;
    qualifier->typeQualifier.funcQual.paramCount = paramCount;

    return qualifier;
}

static Node *handleTypeQualifier(Parser *p) //starts on qualifier, consumes qualifier to wind up on next token after it
{
    if (IS_OPEN_PAREN(p)) return handleFuncQualifier(p);
    if (IS_OPEN_BRACKET(p)) return handleArrayQualifier(p);

    if (IS_OP(p, "*"))
    {
        Node *qualifier = makeSimpleQualifier(SQ_POINTER, QUAL_POINTER);
        skip(p, "advancing past * in type");
        return qualifier;
    }
    if (IS_KEYWORD(p, "const"))
    {
        Node *qualifier = makeSimpleQualifier(SQ_CONST, QUAL_CONST);
        skip(p, "advancing past const in type");
        return qualifier;
    }
    if (IS_KEYWORD(p, "volatile"))
    {
        Node *qualifier = makeSimpleQualifier(SQ_VOLATILE, QUAL_VOLATILE);
        skip(p, "advancing past volatile in type");
        return qualifier;
    }

    return NULL;
}

Node *parseType(Parser *p) // type -> [type (qualifier)], can parse itself through wrapping itself up in a loop!
{
    Node *currentType;
    Token base = CUR(p);
    PrimitiveType primitive;

    if (primitiveFromToken(&base, &primitive))
    {
        currentType = malloc(sizeof(Node));
        currentType->type = NODE_PRIMITIVE;
        currentType->primitiveType.type = primitive;
        skip(p, "expected qualifier or name after type");
    }
    else if (base.type == TOKEN_IDENTIFIER)
    {
        currentType = malloc(sizeof(Node));
        currentType->type = NODE_TYPEREF;
        currentType->typeRef.name = base.text;
        skip(p, "expected qualifier or name after type");
    }
    else
    {
        parserError("Expected type name at position %d, got '%s'", p->pos, base.text);
        return NULL;
    }

    bool hasPreviousQualifier = false;
    QualifierKind previousKind = QUAL_POINTER;

    while (isTypeQualifierStart(p))
    {
        if (IS_OPEN_BRACKET(p) && hasPreviousQualifier && previousKind == QUAL_ARRAY)
            parserError("Adjacent array brackets are not allowed; typedef an array first");

        Node *qualifier = handleTypeQualifier(p);
        Node *wrapped = malloc(sizeof(Node));
        wrapped->type = NODE_TYPE;
        wrapped->typeNode.innerType = currentType;
        wrapped->typeNode.qualifier = qualifier;

        currentType = wrapped;
        previousKind = qualifier->typeQualifier.kind;
        hasPreviousQualifier = true;
    }

    return currentType;
}

Node *handleVarDecl(Parser *p) //create var declaration in the format varDecl -> [type name (expr)]
{
    Node *varDecl = malloc(sizeof(Node));
    varDecl->type = NODE_VAR_DECL;
    varDecl->varDecl.type = parseType(p);

    Node *outerType = varDecl->varDecl.type;
    if (outerType->type == NODE_TYPE &&
        outerType->typeNode.qualifier->typeQualifier.kind == QUAL_FUNCTION)
        parserError("A variable cannot have a bare function type; declare a pointer to function instead");

    Token nameTok = consume(p, TOKEN_IDENTIFIER, NULL, "Expected variable name after type");

    Node *var = malloc(sizeof(Node));
    var->type = NODE_VAR;
    var->var.name = nameTok.text;
    varDecl->varDecl.var = var;

    if (IS_OP(p, "="))
    {
        skip(p, "advancing past = in variable declaration");
        varDecl->varDecl.valueExpr = parseExpression(p, 10);
    }
    else
    {
        varDecl->varDecl.valueExpr = NULL;
    }

    return varDecl;
}

Node *handleTypedef(Parser *p)
{
    Node *typeDecl = malloc(sizeof(Node));
    typeDecl->type = NODE_TYPE_DECL;

    consume(p, TOKEN_KEYWORD, "typedef", "Expected typedef");
    typeDecl->typeDecl.underlyingInstance = parseType(p);

    Token aliasTok = consume(p, TOKEN_IDENTIFIER, NULL, "Expected typedef alias name");
    consume(p, TOKEN_PUNCTUATION, ";", "Expected ; after typedef");
    typeDecl->typeDecl.aliasName = aliasTok.text;

    p->defTypes = realloc(p->defTypes, sizeof(char*) * (p->defTypeCount + 1));
    p->defTypes[p->defTypeCount++] = aliasTok.text;

    return typeDecl;
}

Node *handleParam(Parser *p, bool declaration)
{
    if (declaration) return handleVarDecl(p);
    if (IS_LITERAL(p)) return handleLiteral(p);
    return parseExpression(p, 0);
}

Node **handleArgList(Parser *p, bool declaration, int *outCount) //starts on function name
{
    int paramCap = 8;
    Node **params = malloc(sizeof(Node*) * paramCap);
    int paramCount = 0;

    skip(p, "Expected ( after name to begin argument list"); // step off name, onto (
    consume(p, TOKEN_PUNCTUATION, "(", "Expected ( to begin argument list");

    if (IS_VOID_KW(p) && NEXT_PUNCT(p, ")"))
    {
        skip(p, "advancing past void in parameter list");
    }

    while (!IS_CLOSE_PAREN(p))
    {
        if (p->pos >= p->tokenCount || CUR(p).type == TOKEN_EOF)
            parserError("Expected ) before end of file in parameter list");

        if (paramCount >= paramCap) { paramCap *= 2; params = realloc(params, sizeof(Node*) * paramCap); }
        params[paramCount++] = handleParam(p, declaration);

        if (IS_COMMA(p))
        {
            skip(p, "advancing past , in parameter list");
            continue;
        }
        if (!IS_CLOSE_PAREN(p))
            parserError("Expected , or ) in parameter list at position %d", p->pos);
    }

    consume(p, TOKEN_PUNCTUATION, ")", "Expected ) to close parameter list");
    *outCount = paramCount;
    return params;
}

Node **handleCallArgs(Parser *p, int *outCount) //starts after opening (, consumes closing )
{
    int argCap = 8;
    Node **args = malloc(sizeof(Node*) * argCap);
    int argCount = 0;

    if (!IS_CLOSE_PAREN(p))
    {
        while (1)
        {
            if (argCount >= argCap) { argCap *= 2; args = realloc(args, sizeof(Node*) * argCap); }
            args[argCount] = parseExpression(p, 10); //rbp 10 stops before comma
            argCount++;
            if (!IS_COMMA(p)) break;
            skip(p, "Expected another argument after , before end of file");
        }
    }
    consume(p, TOKEN_PUNCTUATION, ")", "Expected ) to close call expression");
    *outCount = argCount;
    return args;
}

Node* ledCall(Parser *p, Node *left) //build call expression, ( was already skipped
{
    Node *exp = malloc(sizeof(Node));

    exp->type = NODE_CALL_EXPR;
    exp->callExpr.callee = left;
    exp->callExpr.args = handleCallArgs(p, &exp->callExpr.argCount);
    return exp;
}

Node* nudCast(Parser *p)
{
    Node *castExp = malloc(sizeof(Node));

    castExp->type = NODE_CAST;
    castExp->cast.type = parseType(p);
    consume(p, TOKEN_PUNCTUATION, ")", "expected closing paren on cast");
    castExp->cast.exp = parseExpression(p, 139);
    return castExp;
}

Node* nudParen(Parser *p) //distinguish cast from parenthesized expression
{
    if (isTypeName(p, &CUR(p))) return nudCast(p); //( was already skipped

    Node *exp = parseExpression(p, 0);
    consume(p, TOKEN_PUNCTUATION, ")", "Expected ) to close parenthesized expression");
    return exp;
}

Node* nudUnary(Parser *p) //build unary expression with prefix operator
{
    Node *exp = malloc(sizeof(Node));

    Token opToken = PREV(p); //prefix token was already skipped by parseExpression
    OperatorType opType = lookupOperator(opToken); //get operator type of current operator
    Node *op = malloc(sizeof(Node));

    op->type = NODE_OPERATOR;
    op->operator.opType = opType;
    int rbp = rules[opType].operator.lbp - 1; //prefix unary is always right assoc

    exp->type = NODE_UNARY_EXPR;
    exp->unaryExpr.operator = op;
    exp->unaryExpr.exp = parseExpression(p, rbp);

    return exp;
}

Node* nudAddress(Parser *p) //build address-of expression with prefix &
{
    Node *exp = malloc(sizeof(Node));

    Token opToken = PREV(p); //prefix token was already skipped by parseExpression
    OperatorType opType = lookupOperator(opToken);
    Node *op = malloc(sizeof(Node));

    op->type = NODE_OPERATOR;
    op->operator.opType = opType;

    exp->type = NODE_UNARY_EXPR;
    exp->unaryExpr.operator = op;
    exp->unaryExpr.exp = parseExpression(p, 139); //bind like a unary, not bitwise AND

    return exp;
}

Node* nudPrimary(Parser *p) //build literal or var node from current token
{
    Token tok = CUR(p);

    Node *node = malloc(sizeof(Node));
    if (IS_LITERAL(p))
    {
        node = handleLiteral(p);
    }
    else // variable
    {
        node->type = NODE_VAR;
        node->var.name = tok.text;
        skip(p, "advancing past identifier");
    }
    return node;
}

Node* ledUnary(Parser *p, Node *left) //build unary expression with postfix operator
{
    Node *exp = malloc(sizeof(Node));

    Token opToken = PREV(p); //postfix token was already skipped by parseExpression
    OperatorType opType = lookupOperator(opToken);
    Node *op = malloc(sizeof(Node));

    op->type = NODE_OPERATOR;
    op->operator.opType = opType;

    exp->type = NODE_UNARY_EXPR;
    exp->unaryExpr.operator = op;
    exp->unaryExpr.exp = left;

    return exp;
}

Node* ledBinaryLeft(Parser *p, Node *left) //build new left, starts on right of op, creates binary expression with left, operator, right
{
    Node *exp = malloc(sizeof(Node)); //allocate expression node

    Token opToken = PREV(p); //operator token is token just skipped
    OperatorType opType = lookupOperator(opToken); //get operator type of current operator
    Node *op = malloc(sizeof(Node));

    op->type = NODE_OPERATOR;
    op->operator.opType = opType;
    int rbp = rules[opType].operator.lbp; 

    exp->type = NODE_BINARY_EXPR;
    exp->binaryExpr.leftExp = left; //given left is left of binary exp
    exp->binaryExpr.operator = op; //set operator to current operator
    exp->binaryExpr.rightExp = parseExpression(p, rbp); //parse right of exp

    return exp; //return binary expression as new left
}

Node* ledBinaryRight(Parser *p, Node *left) //right assoc version
{ //copying the exact same function with a single literal edit because i couldnt be bothered to generalize the binary function is a bit of a disgusting kludge and im sorry
    Node *exp = malloc(sizeof(Node));

    Token opToken = PREV(p);
    OperatorType opType = lookupOperator(opToken);
    Node *op = malloc(sizeof(Node));

    op->type = NODE_OPERATOR;
    op->operator.opType = opType;
    int rbp = rules[opType].operator.lbp - 1; 

    exp->type = NODE_BINARY_EXPR;
    exp->binaryExpr.leftExp = left; 
    exp->binaryExpr.operator = op; 
    exp->binaryExpr.rightExp = parseExpression(p, rbp);

    return exp; //return binary expression as new left
}

Node* ledTernary(Parser *p, Node *left) //build ternary expression, ? was already skipped
{
    Node *exp = malloc(sizeof(Node));

    Token opToken = PREV(p);
    OperatorType opType = lookupOperator(opToken);
    Node *op = malloc(sizeof(Node));

    op->type = NODE_OPERATOR;
    op->operator.opType = opType;

    exp->type = NODE_TERNARY_EXPR;
    exp->ternaryExpr.operator = op;
    exp->ternaryExpr.conditionExp = left;
    exp->ternaryExpr.ifTrueExp = parseExpression(p, 0); //stops at :
    consume(p, TOKEN_PUNCTUATION, ":", "Expected : in ternary expression");
    exp->ternaryExpr.ifFalseExp = parseExpression(p, 29); //right assoc

    return exp;
}

Node* ledMember(Parser *p, Node *left) //build member access, . or -> was already skipped
{
    Node *exp = malloc(sizeof(Node));

    Token opToken = PREV(p);
    bool isArrow = !strcmp(opToken.text, "->");
    Token member = consume(p, TOKEN_IDENTIFIER, NULL, "Expected member name after . or ->");

    exp->type = NODE_MEMBER_EXPR;
    exp->memberExpr.object = left;
    exp->memberExpr.member = member.text;
    exp->memberExpr.isArrow = isArrow;

    return exp;
}

Node* ledIndex(Parser *p, Node *left) //build index expression, [ was already skipped
{
    Node *exp = malloc(sizeof(Node));

    exp->type = NODE_INDEX_EXPR;
    exp->indexExpr.target = left;
    exp->indexExpr.index = parseExpression(p, 0);
    consume(p, TOKEN_PUNCTUATION, "]", "Expected ] to close index expression");

    return exp;
}

Node *parseExpression(Parser *p, int rbp) //parses the actual stream of expression tokens
{
    Token tok = CUR(p);
    OperatorType opType = lookupOperator(tok); // maps Token -> OperatorType

    Node *left = NULL; //no allocation because pointer gets overriden
    if (opType != OPERATOR_NONE && rules[opType].operator.nud != NULL) //consume operator
    {
        skip(p, "advancing past prefix/nud token");
        left = rules[opType].operator.nud(p);
    }
    else if (IS_LITERAL(p)|| IS_IDENTIFIER(p)) //consume literal/variable
    {
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

Node *handleForeverStatement(Parser *p) {
    Node* foreverStmt = malloc(sizeof(Node));
    foreverStmt->type = NODE_FOREVER_STMT;
    consume(p, TOKEN_KEYWORD, "forever", "expected forever statement");
    foreverStmt->foreverLoop.logic = handleStatement(p);
    return foreverStmt;
}

Node *handleIfStatement(Parser *p)
{
    Node* statement = malloc(sizeof(Node));
    if (IS_KEYWORD(p, "if"))
    {
        statement->type = NODE_IF_STMT;
        consume(p, TOKEN_KEYWORD, "if", "expected if statement");
        consume(p, TOKEN_PUNCTUATION, "(", "Expected ( after if"); //consume paren so ) fallthrough means it will stop on final ), leaving it as cur
        statement->ifStmt.expr = parseExpression(p, 0);
        consume(p, TOKEN_PUNCTUATION, ")", "Expected ) after if condition");
        statement->ifStmt.logic = handleStatement(p);
        if(IS_KEYWORD(p, "else") || IS_KEYWORD(p, "elif")) statement->ifStmt.elseStmt = handleIfStatement(p);
        else statement->ifStmt.elseStmt = NULL;
    }
    else if (IS_KEYWORD(p, "else"))
    {
        statement->type = NODE_ELSE_STMT;
        consume(p, TOKEN_KEYWORD, "else", "expected else statement");
        statement->elseStmt.logic = handleStatement(p);
    }
    else if (IS_KEYWORD(p, "elif")) //another terrible repetitive kludge because i am too lazy to refactor
    {
        statement->type = NODE_ELSE_STMT;
        consume(p, TOKEN_KEYWORD, "elif", "expected elif statement");
        Node* elifStmt = malloc(sizeof(Node)); 
        elifStmt->type = NODE_IF_STMT; 
        statement->elseStmt.logic = elifStmt; //wrap if statement in else statement
        consume(p, TOKEN_PUNCTUATION, "(", "Expected ( after elif");
        elifStmt->ifStmt.expr = parseExpression(p, 0);
        consume(p, TOKEN_PUNCTUATION, ")", "Expected ) after elif condition");
        elifStmt->ifStmt.logic = handleStatement(p);
        if(IS_KEYWORD(p, "else") || IS_KEYWORD(p, "elif")) elifStmt->ifStmt.elseStmt = handleIfStatement(p);
        else elifStmt->ifStmt.elseStmt = NULL;
    }
    return statement;
}

Node *handleExpressionStatement(Parser *p)
{
    Node *expressionStatement = malloc(sizeof(Node));
    expressionStatement->type = NODE_EXPR_STMT;
    expressionStatement->exprStmt.expr = parseExpression(p, 0);
    consume(p, TOKEN_PUNCTUATION, ";", "Expected ; to terminate statement!");
    return expressionStatement;
}

Node *handleStatement(Parser *p) //starts on whatever is after ; or }
{
    //logic for determining kind of statement will go here
    //STUB, not all statements implemented.
    if (IS_OPEN_BRACE(p)) return handleCodeBlock(p);
    if (IS_KEYWORD(p, "if")) return handleIfStatement(p);
    if (IS_KEYWORD(p, "else") || IS_KEYWORD(p, "elif")) parserError("Expected opening if statement to else or elif at position %d", p->pos);
    if (IS_KEYWORD(p, "forever")) return handleForeverStatement(p);
    if (IS_KEYWORD(p, "typedef")) return handleTypedef(p);
    if (isTypeName(p, &CUR(p)))
    {
        Node *decl = handleVarDecl(p);
        consume(p, TOKEN_PUNCTUATION, ";", "Expected ; after variable declaration");
        return decl;
    }
    return handleExpressionStatement(p);
}

Node *handleCodeBlock(Parser *p) //starts on opening {, ends after closing }
{
    Node *block = malloc(sizeof(Node));
    block->type = NODE_BLOCK;

    int statementCap = 8;
    Node **statements = malloc(sizeof(Node*) * statementCap);
    int statementCount = 0;

    consume(p, TOKEN_PUNCTUATION, "{", "Expected opening { for code block");

    while (!IS_CLOSE_BRACE(p))
    {
        if (p->pos >= p->tokenCount || CUR(p).type == TOKEN_EOF)
            parserError("Expected } before end of file");

        if (statementCount >= statementCap)
        {
            statementCap *= 2;
            statements = realloc(statements, sizeof(Node*) * statementCap);
        }
        statements[statementCount++] = handleStatement(p);
    }

    consume(p, TOKEN_PUNCTUATION, "}", "Expected } at end of code block");

    block->block.statements = statements;
    block->block.count = statementCount;
    return block;
}

char *handleImport(Parser *p)
{
    consume(p, TOKEN_KEYWORD, "import", "Expected import");
    Token nameTok = consume(p, TOKEN_IDENTIFIER, NULL, "Expected import name");
    consume(p, TOKEN_PUNCTUATION, ";", "Expected ; after import name");
    return nameTok.text;
}

Node *handleGlobal(Parser *p)
{
    Node *global = handleVarDecl(p);
    consume(p, TOKEN_PUNCTUATION, ";", "Expected ; after global declaration");
    return global;
}

Node *handleFunction(Parser *p)
{
    Node *function = malloc(sizeof(Node));
    function->type = NODE_FUNCTION_DECL;
    function->functionDecl.name = CUR(p).text;

    if (p->pos > 0 &&
        (p->tokens[p->pos - 1].type == TOKEN_KEYWORD ||
         p->tokens[p->pos - 1].type == TOKEN_IDENTIFIER))
        function->functionDecl.returnType = p->tokens[p->pos - 1].text;
    else
        function->functionDecl.returnType = "void";

    int paramCount;
    Node **params = handleArgList(p, true, &paramCount);
    function->functionDecl.paramCount = paramCount;
    function->functionDecl.params = params;

    if (!IS_OPEN_BRACE(p)) parserError("Expected { to begin function body at position %d", p->pos);
    function->functionDecl.body = handleCodeBlock(p);
    return function;
}

Node *parse(Parser *p) //STUB: ADD PREPROCESSOR DIRECTIVES | main parsing loop
{
    int importCount = 0;
    int functionCount = 0;
    int globalCount = 0;

    int importCap = 8, functionCap = 8, globalCap = 8;
    char **imports = malloc(sizeof(char*) * importCap);
    Node **functions = malloc(sizeof(Node*) * functionCap);
    Node **globals = malloc(sizeof(Node*) * globalCap);

    while (CUR(p).type != TOKEN_EOF)
    {
        if (IS_KEYWORD(p, "import"))
        {
            if (importCount >= importCap) { importCap *= 2; imports = realloc(imports, sizeof(char*) * importCap); }
            imports[importCount++] = handleImport(p);
            continue;
        }

        if (IS_KEYWORD(p, "typedef"))
        {
            if (globalCount >= globalCap) { globalCap *= 2; globals = realloc(globals, sizeof(Node*) * globalCap); }
            globals[globalCount++] = handleTypedef(p);
            continue;
        }

        if (IS_IDENTIFIER(p) && NEXT_PUNCT(p, "("))
        {
            if (functionCount >= functionCap) { functionCap *= 2; functions = realloc(functions, sizeof(Node*) * functionCap); }
            functions[functionCount++] = handleFunction(p);
            continue;
        }

        if (isTypeName(p, &CUR(p)))
        {
            bool isSimpleFunction =
                NEXT_IDENTIFIER(p) &&
                p->pos + 2 < p->tokenCount &&
                p->tokens[p->pos + 2].type == TOKEN_PUNCTUATION &&
                !strcmp(p->tokens[p->pos + 2].text, "(");

            if (isSimpleFunction)
            {
                p->pos++;
                continue;
            }

            if (globalCount >= globalCap) { globalCap *= 2; globals = realloc(globals, sizeof(Node*) * globalCap); }
            globals[globalCount++] = handleGlobal(p);
            continue;
        }

        parserError("Unexpected token '%s' at top level, position %d", CUR(p).text, p->pos);
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