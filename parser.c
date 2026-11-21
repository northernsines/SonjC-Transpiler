#include "parser.h"
#include <stdlib.h>
#include <string.h>

/*
SonjC Parser
Written Aug 2026
Parses tokens emitted by the lexer into an Abstract Syntax Tree.
*/

Node rules[60];
Node* parseExpression(Parser*, int);
Node* handleStatement(Parser*);
Node* handleCodeBlock(Parser*);
Node* parseType(Parser*);
Node* handleTypedef(Parser*);
Node* handleGlobal(Parser*);
Node* handleObjectDecl(Parser*, bool);
Node* handleFunction(Parser*, Node*);
Node* handleVarDeclWithType(Parser*, Node*);

//create literal node, Q16.16 encoding algorithm
Node* handleLiteral(Parser* p) {
    Token t = CUR(p);
    Node* literal = malloc(sizeof(Node));
    switch (t.type) //map tokentype to node types for literals
    {
        case TOKEN_INTEGER:
            literal->type = NODE_INT_LITERAL;
            break;
        case TOKEN_FLOAT:
            literal->type = NODE_FLOAT_LITERAL;
            break;
        case TOKEN_FIXED:
            literal->type = NODE_FIXED_LITERAL;
            break;
        case TOKEN_CHAR:
            literal->type = NODE_CHAR_LITERAL;
            break;
        case TOKEN_STRING:
            literal->type = NODE_STRING_LITERAL;
            break;
        case TOKEN_KEYWORD:
            literal->type = NODE_BOOL_LITERAL;
            break;
        default:
            parserError("Expected literal in literal slot at position %d", p->pos);
    }
    char* str = malloc(strlen(t.text) + 1);
    if (str == NULL)
        parserError("malloc for string literal allocation returned null");
    strcpy(str, t.text);
    int str_size = strlen(str);
    if (literal->type == NODE_INT_LITERAL) //convert values to proper types
    {
        literal->int_literal.value = atoi(str);
    } else if (literal->type == NODE_FLOAT_LITERAL) {
        literal->float_literal.value = atof(str);
    } else if (literal->type == NODE_FIXED_LITERAL) // Q16.16 fixed point algorithm
    {
        if (str_size > 0 && str[str_size - 1] == 'f') //drop the fixed marker before splitting the number
        {
            str[--str_size] = '\0';
        }

        int i;
        char* int_part_str = malloc(strlen(str) + 1);
        char* frac_part_str = malloc(strlen(str) + 1);
        for (i = 0; i < str_size; i++) //iterate until detect . build left string
        {
            char ch = str[i];
            bool is_dot = ch == '.';
            if (is_dot)
                break;
            else {
                int_part_str[i] = ch;
            }
        }
        int_part_str[i] = '\0';
        i++; //skip dot
        int ioffset = i;
        for (i = ioffset; i < str_size; i++) //iterate until null term, build right string
        {
            char ch = str[i];
            bool is_null = ch == '\0';
            if (is_null)
                break;
            else {
                frac_part_str[i - ioffset] = ch;
            }
        }
        frac_part_str[i - ioffset] = '\0';
        uint64_t int_part = strtoull(int_part_str, NULL, 10);
        uint64_t frac_digits = strtoull(frac_part_str, NULL, 10);
        size_t frac_len = strlen(frac_part_str);

        __uint128_t scaled = (__uint128_t)frac_digits << 16;
        uint32_t frac_part = (uint32_t)(scaled / pow10(frac_len)); // pow10 = your own int pow

        uint32_t underlying = ((uint32_t)int_part << 16) | frac_part;
        literal->fixed_literal.value = underlying;
    } else if (literal->type == NODE_CHAR_LITERAL) {
        literal->char_literal.value = *str;
    } else if (literal->type == NODE_STRING_LITERAL) {
        literal->string_literal.value = str;
    } else if (literal->type == NODE_BOOL_LITERAL) {
        if (!strcmp(str, "true"))
            literal->bool_literal.value = true;
        else
            literal->bool_literal.value = false;
    }
    skip(p, "advancing past literal");
    return literal;
}

//check if simple tag qualifier
static bool isTypeQualifierStart(Parser* p) {
    return IS_OPEN_PAREN(p) || IS_OPEN_BRACKET(p) ||
           IS_OP(p, "*") ||
           IS_KEYWORD(p, "const") || IS_KEYWORD(p, "volatile") ||
           IS_KEYWORD(p, "raw") || IS_KEYWORD(p, "unsigned") ||
           IS_KEYWORD(p, "long") || IS_KEYWORD(p, "short");
}

//simple tag qualifier that doesn't contain further data
static Node* makeSimpleQualifier(SimpleQualifier simple_qual, QualifierKind kind) {
    Node* qualifier = malloc(sizeof(Node));
    qualifier->type = NODE_TYPE_QUALIFIER;
    qualifier->type_qualifier.kind = kind;
    qualifier->type_qualifier.simple_qual = simple_qual;
    return qualifier;
}

//bare primitive type node
static Node* makePrimitiveType(PrimitiveType primitive) {
    Node* type = malloc(sizeof(Node));
    type->type = NODE_PRIMITIVE;
    type->primitive_type.type = primitive;
    return type;
}

//track user-defined type names for isTypeName
static void registerDefType(Parser* p, char* name) {
    p->def_types = realloc(p->def_types, sizeof(char*) * (p->def_type_count + 1));
    p->def_types[p->def_type_count++] = name;
}

//skip qualifiers that still describe the base type
static Node* innermostType(Node* type) {
    while (type && type->type == NODE_TYPE) {
        QualifierKind kind = type->type_node.qualifier->type_qualifier.kind;
        if (kind == QUAL_POINTER || kind == QUAL_ARRAY || kind == QUAL_FUNCTION)
            break;
        type = type->type_node.inner_type;
    }
    return type;
}

//qualifier for stack array
static Node* handleArrayQualifier(Parser* p) {
    Node* qualifier = malloc(sizeof(Node));
    qualifier->type = NODE_TYPE_QUALIFIER;
    qualifier->type_qualifier.kind = QUAL_ARRAY;

    consume(p, TOKEN_PUNCTUATION, "[", "Expected [ to begin array type");
    Token size_tok = consume(p, TOKEN_INTEGER, NULL, "Expected integer array size");
    qualifier->type_qualifier.array_size = atoi(size_tok.text);
    consume(p, TOKEN_PUNCTUATION, "]", "Expected ] after array size");

    return qualifier;
}

//type qualifier for function pointer
static Node* handleFuncQualifier(Parser* p) {
    Node* qualifier = malloc(sizeof(Node));
    qualifier->type = NODE_TYPE_QUALIFIER;
    qualifier->type_qualifier.kind = QUAL_FUNCTION;

    int param_cap = 8;
    int param_count = 0;
    Node** params = malloc(sizeof(Node*) * param_cap);

    consume(p, TOKEN_PUNCTUATION, "(", "Expected ( to begin function type");

    if (IS_CLOSE_PAREN(p)) {
        consume(p, TOKEN_PUNCTUATION, ")", "Expected ) to close function type");
    } else if (IS_VOID_KW(p) && NEXT_PUNCT(p, ")")) {
        skip(p, "advancing past void in function type");
        consume(p, TOKEN_PUNCTUATION, ")", "Expected ) to close function type");
    } else {
        while (1) {
            if (param_count >= param_cap) {
                param_cap *= 2;
                params = realloc(params, sizeof(Node*) * param_cap);
            }

            params[param_count++] = parseType(p);

            if (IS_COMMA(p)) {
                skip(p, "advancing past , in function type");
                continue;
            }
            break;
        }

        consume(p, TOKEN_PUNCTUATION, ")", "Expected ) to close function type");
    }

    qualifier->type_qualifier.func_qual.params = params;
    qualifier->type_qualifier.func_qual.param_count = param_count;

    return qualifier;
}

//starts on qualifier, consumes qualifier to wind up on next token after it
static Node* handleTypeQualifier(Parser* p) {
    if (IS_OPEN_PAREN(p))
        return handleFuncQualifier(p);
    if (IS_OPEN_BRACKET(p))
        return handleArrayQualifier(p);

    if (IS_OP(p, "*")) {
        Node* qualifier = makeSimpleQualifier(SQ_POINTER, QUAL_POINTER);
        skip(p, "advancing past * in type");
        return qualifier;
    }
    if (IS_KEYWORD(p, "const")) {
        Node* qualifier = makeSimpleQualifier(SQ_CONST, QUAL_CONST);
        skip(p, "advancing past const in type");
        return qualifier;
    }
    if (IS_KEYWORD(p, "volatile")) {
        Node* qualifier = makeSimpleQualifier(SQ_VOLATILE, QUAL_VOLATILE);
        skip(p, "advancing past volatile in type");
        return qualifier;
    }
    if (IS_KEYWORD(p, "raw")) {
        Node* qualifier = makeSimpleQualifier(SQ_RAW, QUAL_RAW);
        skip(p, "advancing past raw in type");
        return qualifier;
    }
    if (IS_KEYWORD(p, "unsigned")) {
        Node* qualifier = makeSimpleQualifier(SQ_UNSIGNED, QUAL_UNSIGNED);
        skip(p, "advancing past unsigned in type");
        return qualifier;
    }
    if (IS_KEYWORD(p, "long")) {
        Node* qualifier = makeSimpleQualifier(SQ_LONG, QUAL_LONG);
        skip(p, "advancing past long in type");
        return qualifier;
    }
    if (IS_KEYWORD(p, "short")) {
        Node* qualifier = makeSimpleQualifier(SQ_SHORT, QUAL_SHORT);
        skip(p, "advancing past short in type");
        return qualifier;
    }

    return NULL;
}

// type -> [type (qualifier)], can parse itself through wrapping itself up in a loop!
Node* parseType(Parser* p) {
    Node* current_type;
    Token base = CUR(p);
    PrimitiveType primitive;

    if (primitiveFromToken(&base, &primitive)) {
        current_type = makePrimitiveType(primitive);
        skip(p, "expected qualifier or name after type");
    } else if (base.type == TOKEN_IDENTIFIER) {
        current_type = malloc(sizeof(Node));
        current_type->type = NODE_TYPEREF;
        current_type->type_ref.name = base.text;
        skip(p, "expected qualifier or name after type");
    } else {
        parserError("Expected type name at position %d, got '%s'", p->pos, base.text);
        return NULL;
    }

    bool has_previous_qualifier = false;
    QualifierKind previous_kind = QUAL_POINTER;

    while (isTypeQualifierStart(p)) {
        if (IS_OPEN_BRACKET(p) && has_previous_qualifier && previous_kind == QUAL_ARRAY)
            parserError("Adjacent array brackets are not allowed; typedef an array first");

        Node* qualifier = handleTypeQualifier(p);
        QualifierKind kind = qualifier->type_qualifier.kind;

        if (kind == QUAL_RAW &&
            !(current_type->type == NODE_TYPE &&
              current_type->type_node.qualifier->type_qualifier.kind == QUAL_POINTER))
            parserError("raw must qualify a pointer at position %d", p->pos);

        if (kind == QUAL_UNSIGNED || kind == QUAL_LONG || kind == QUAL_SHORT) {
            Node* base_type = innermostType(current_type);
            if (!base_type || base_type->type != NODE_PRIMITIVE)
                parserError("unsigned, long, and short must qualify a base type at position %d", p->pos);
        }

        Node* wrapped = malloc(sizeof(Node));
        wrapped->type = NODE_TYPE;
        wrapped->type_node.inner_type = current_type;
        wrapped->type_node.qualifier = qualifier;

        current_type = wrapped;
        previous_kind = kind;
        has_previous_qualifier = true;
    }

    return current_type;
}

//var declaration with an already parsed type, starts on name
Node* handleVarDeclWithType(Parser* p, Node* type) {
    Node* var_decl = malloc(sizeof(Node));
    var_decl->type = NODE_VAR_DECL;
    var_decl->var_decl.type = type;

    Node* outer_type = var_decl->var_decl.type;
    if (outer_type->type == NODE_TYPE &&
        outer_type->type_node.qualifier->type_qualifier.kind == QUAL_FUNCTION)
        parserError("A variable cannot have a bare function type; declare a pointer to function instead");

    Token name_tok = consume(p, TOKEN_IDENTIFIER, NULL, "Expected variable name after type");

    Node* var = malloc(sizeof(Node));
    var->type = NODE_VAR;
    var->var.name = name_tok.text;
    var_decl->var_decl.var = var;

    if (IS_OP(p, "=")) {
        skip(p, "advancing past = in variable declaration");
        var_decl->var_decl.value_expr = parseExpression(p, 10);
    } else {
        var_decl->var_decl.value_expr = NULL;
    }

    return var_decl;
}

//create var declaration in the format var_decl -> [type name (expr)]
Node* handleVarDecl(Parser* p) {
    return handleVarDeclWithType(p, parseType(p));
}

//field or method, starts on member type
static Node* handleObjectMember(Parser* p) {
    Node* type = parseType(p);

    if (IS_IDENTIFIER(p) && NEXT_PUNCT(p, "("))
        return handleFunction(p, type); //p still sits on the method name

    Node* field = handleVarDeclWithType(p, type);
    consume(p, TOKEN_PUNCTUATION, ";", "Expected ; after field declaration");
    return field;
}

//object type/template, leaves pos after closing }
Node* handleObjectDecl(Parser* p, bool requireName) {
    Node* object = malloc(sizeof(Node));
    object->type = NODE_OBJECT_DECL;
    object->object_decl.name = NULL;

    consume(p, TOKEN_KEYWORD, "object", "Expected object");

    if (IS_IDENTIFIER(p)) {
        Token name_tok = consume(p, TOKEN_IDENTIFIER, NULL, "Expected object name");
        object->object_decl.name = name_tok.text;
        registerDefType(p, name_tok.text); //register before the body so members can self-reference
    } else if (requireName) {
        parserError("Expected object name at position %d", p->pos);
    }

    consume(p, TOKEN_PUNCTUATION, "{", "Expected { to begin object body");

    int member_cap = 8;
    Node** members = malloc(sizeof(Node*) * member_cap);
    int member_count = 0;

    while (!IS_CLOSE_BRACE(p)) {
        if (p->pos >= p->token_count || CUR(p).type == TOKEN_EOF)
            parserError("Expected } before end of file in object body");

        if (member_count >= member_cap) {
            member_cap *= 2;
            members = realloc(members, sizeof(Node*) * member_cap);
        }
        members[member_count++] = handleObjectMember(p);
    }

    consume(p, TOKEN_PUNCTUATION, "}", "Expected } to close object body");

    object->object_decl.members = members;
    object->object_decl.member_count = member_count;
    return object;
}

//type aliasing statement
Node* handleTypedef(Parser* p) {
    Node* type_decl = malloc(sizeof(Node));
    type_decl->type = NODE_TYPE_DECL;

    consume(p, TOKEN_KEYWORD, "typedef", "Expected typedef");

    if (IS_KEYWORD(p, "object")) //object templates can be anonymous or named inside the typedef
    {
        type_decl->type_decl.underlying_instance = handleObjectDecl(p, false);

        Token alias_tok = consume(p, TOKEN_IDENTIFIER, NULL, "Expected typedef alias name");
        consume(p, TOKEN_PUNCTUATION, ";", "Expected ; after typedef");
        type_decl->type_decl.alias_name = alias_tok.text;
        registerDefType(p, alias_tok.text);

        return type_decl;
    }

    type_decl->type_decl.underlying_instance = parseType(p);

    Token alias_tok = consume(p, TOKEN_IDENTIFIER, NULL, "Expected typedef alias name");
    consume(p, TOKEN_PUNCTUATION, ";", "Expected ; after typedef");
    type_decl->type_decl.alias_name = alias_tok.text;

    registerDefType(p, alias_tok.text);

    return type_decl;
}

//redirect to proper function
Node* handleParam(Parser* p, bool declaration) {
    if (declaration)
        return handleVarDecl(p);
    if (IS_LITERAL(p))
        return handleLiteral(p);
    return parseExpression(p, 0);
}

//starts on function name
Node** handleArgList(Parser* p, bool declaration, int* outCount) {
    int param_cap = 8;
    Node** params = malloc(sizeof(Node*) * param_cap);
    int param_count = 0;

    skip(p, "Expected ( after name to begin argument list"); // step off name, onto (
    consume(p, TOKEN_PUNCTUATION, "(", "Expected ( to begin argument list");

    if (IS_VOID_KW(p) && NEXT_PUNCT(p, ")")) {
        skip(p, "advancing past void in parameter list");
    }

    while (!IS_CLOSE_PAREN(p)) {
        if (p->pos >= p->token_count || CUR(p).type == TOKEN_EOF)
            parserError("Expected ) before end of file in parameter list");

        if (param_count >= param_cap) {
            param_cap *= 2;
            params = realloc(params, sizeof(Node*) * param_cap);
        }
        params[param_count++] = handleParam(p, declaration);

        if (IS_COMMA(p)) {
            skip(p, "advancing past , in parameter list");
            continue;
        }
        if (!IS_CLOSE_PAREN(p))
            parserError("Expected , or ) in parameter list at position %d", p->pos);
    }

    consume(p, TOKEN_PUNCTUATION, ")", "Expected ) to close parameter list");
    *outCount = param_count;
    return params;
}

//starts after opening (, consumes closing )
Node** handleCallArgs(Parser* p, int* outCount) {
    int arg_cap = 8;
    Node** args = malloc(sizeof(Node*) * arg_cap);
    int arg_count = 0;

    if (!IS_CLOSE_PAREN(p)) {
        while (1) {
            if (arg_count >= arg_cap) {
                arg_cap *= 2;
                args = realloc(args, sizeof(Node*) * arg_cap);
            }
            args[arg_count] = parseExpression(p, 10); //rbp 10 stops before comma
            arg_count++;
            if (!IS_COMMA(p))
                break;
            skip(p, "Expected another argument after , before end of file");
        }
    }
    consume(p, TOKEN_PUNCTUATION, ")", "Expected ) to close call expression");
    *outCount = arg_count;
    return args;
}

//build call expression, ( was already skipped
Node* ledCall(Parser* p, Node* left) {
    Node* exp = malloc(sizeof(Node));

    exp->type = NODE_CALL_EXPR;
    exp->call_expr.callee = left;
    exp->call_expr.args = handleCallArgs(p, &exp->call_expr.arg_count);
    return exp;
}

//build cast expression
Node* nudCast(Parser* p) {
    Node* cast_exp = malloc(sizeof(Node));

    cast_exp->type = NODE_CAST;
    cast_exp->cast.type = parseType(p);
    consume(p, TOKEN_PUNCTUATION, ")", "expected closing paren on cast");
    cast_exp->cast.exp = parseExpression(p, 139);
    return cast_exp;
}

//distinguish cast from parenthesized expression
Node* nudParen(Parser* p) {
    if (isTypeName(p, &CUR(p)))
        return nudCast(p); //( was already skipped

    Node* exp = parseExpression(p, 0);
    consume(p, TOKEN_PUNCTUATION, ")", "Expected ) to close parenthesized expression");
    return exp;
}

//build unary expression with prefix operator
Node* nudUnary(Parser* p) {
    Node* exp = malloc(sizeof(Node));

    Token op_token = PREV(p);                       //prefix token was already skipped by parseExpression
    OperatorType op_type = lookupOperator(op_token); //get operator type of current operator
    Node* op = malloc(sizeof(Node));

    op->type = NODE_OPERATOR;
    op->operator.op_type = op_type;
    int rbp = rules[op_type].operator.lbp - 1; //prefix unary is always right assoc

    exp->type = NODE_UNARY_EXPR;
    exp->unary_expr.operator = op;
    exp->unary_expr.exp = parseExpression(p, rbp);

    return exp;
}

//build sizeof expression, operand may be (type) or an expression
Node* nudSizeof(Parser* p) {
    Node* exp = malloc(sizeof(Node));
    Node* op = malloc(sizeof(Node));

    op->type = NODE_OPERATOR;
    op->operator.op_type = OPERATOR_SIZEOF;

    exp->type = NODE_UNARY_EXPR;
    exp->unary_expr.operator = op;

    if (IS_OPEN_PAREN(p)) {
        int open_pos = p->pos;
        skip(p, "advancing past ( in sizeof");

        if (isTypeName(p, &CUR(p))) {
            Node* type = parseType(p);
            if (IS_CLOSE_PAREN(p)) {
                skip(p, "advancing past ) in sizeof");
                exp->unary_expr.exp = type;
                return exp;
            }
            p->pos = open_pos; //not a type operand, reparse as an expression
        }

        exp->unary_expr.exp = parseExpression(p, 0);
        consume(p, TOKEN_PUNCTUATION, ")", "Expected ) to close sizeof expression");
    } else {
        exp->unary_expr.exp = parseExpression(p, 139);
    }

    return exp;
}

//build address-of expression with prefix &
Node* nudAddress(Parser* p) {
    Node* exp = malloc(sizeof(Node));

    Token op_token = PREV(p); //prefix token was already skipped by parseExpression
    OperatorType op_type = lookupOperator(op_token);
    Node* op = malloc(sizeof(Node));

    op->type = NODE_OPERATOR;
    op->operator.op_type = op_type;

    exp->type = NODE_UNARY_EXPR;
    exp->unary_expr.operator = op;
    exp->unary_expr.exp = parseExpression(p, 139); //bind like a unary, not bitwise AND

    return exp;
}

//build literal or var node from current token
Node* nudPrimary(Parser* p) {
    Token tok = CUR(p);

    Node* node = malloc(sizeof(Node));
    if (IS_LITERAL(p)) {
        node = handleLiteral(p);
    } else // variable
    {
        node->type = NODE_VAR;
        node->var.name = tok.text;
        skip(p, "advancing past identifier");
    }
    return node;
}

//build unary expression with postfix operator
Node* ledUnary(Parser* p, Node* left) {
    Node* exp = malloc(sizeof(Node));

    Token op_token = PREV(p); //postfix token was already skipped by parseExpression
    OperatorType op_type = lookupOperator(op_token);
    Node* op = malloc(sizeof(Node));

    op->type = NODE_OPERATOR;
    op->operator.op_type = op_type;

    exp->type = NODE_UNARY_EXPR;
    exp->unary_expr.operator = op;
    exp->unary_expr.exp = left;

    return exp;
}

//build new left, starts on right of op, creates binary expression with left, operator, right
Node* ledBinaryLeft(Parser* p, Node* left) {
    Node* exp = malloc(sizeof(Node)); //allocate expression node

    Token op_token = PREV(p);                       //operator token is token just skipped
    OperatorType op_type = lookupOperator(op_token); //get operator type of current operator
    Node* op = malloc(sizeof(Node));

    op->type = NODE_OPERATOR;
    op->operator.op_type = op_type;
    int rbp = rules[op_type].operator.lbp;

    exp->type = NODE_BINARY_EXPR;
    exp->binary_expr.left_exp = left;                     //given left is left of binary exp
    exp->binary_expr.operator = op;                      //set operator to current operator
    exp->binary_expr.right_exp = parseExpression(p, rbp); //parse right of exp

    return exp; //return binary expression as new left
}

//right assoc version
//copying the exact same function with a single literal edit because i couldnt be bothered to generalize the binary function is a bit of a disgusting kludge and im sorry
Node* ledBinaryRight(Parser* p, Node* left) {
    Node* exp = malloc(sizeof(Node));

    Token op_token = PREV(p);
    OperatorType op_type = lookupOperator(op_token);
    Node* op = malloc(sizeof(Node));

    op->type = NODE_OPERATOR;
    op->operator.op_type = op_type;
    int rbp = rules[op_type].operator.lbp - 1;

    exp->type = NODE_BINARY_EXPR;
    exp->binary_expr.left_exp = left;
    exp->binary_expr.operator = op;
    exp->binary_expr.right_exp = parseExpression(p, rbp);

    return exp; //return binary expression as new left
}

//build ternary expression, ? was already skipped
Node* ledTernary(Parser* p, Node* left) {
    Node* exp = malloc(sizeof(Node));

    Token op_token = PREV(p);
    OperatorType op_type = lookupOperator(op_token);
    Node* op = malloc(sizeof(Node));

    op->type = NODE_OPERATOR;
    op->operator.op_type = op_type;

    exp->type = NODE_TERNARY_EXPR;
    exp->ternary_expr.operator = op;
    exp->ternary_expr.condition_exp = left;
    exp->ternary_expr.if_true_exp = parseExpression(p, 0); //stops at :
    consume(p, TOKEN_PUNCTUATION, ":", "Expected : in ternary expression");
    exp->ternary_expr.if_false_exp = parseExpression(p, 29); //right assoc

    return exp;
}

//build member access, . or -> was already skipped
Node* ledMember(Parser* p, Node* left) {
    Node* exp = malloc(sizeof(Node));

    Token op_token = PREV(p);
    bool is_arrow = !strcmp(op_token.text, "->");
    Token member = consume(p, TOKEN_IDENTIFIER, NULL, "Expected member name after . or ->");

    exp->type = NODE_MEMBER_EXPR;
    exp->member_expr.object = left;
    exp->member_expr.member = member.text;
    exp->member_expr.is_arrow = is_arrow;

    return exp;
}

//build index expression, [ was already skipped
Node* ledIndex(Parser* p, Node* left) {
    Node* exp = malloc(sizeof(Node));

    exp->type = NODE_INDEX_EXPR;
    exp->index_expr.target = left;
    exp->index_expr.index = parseExpression(p, 0);
    consume(p, TOKEN_PUNCTUATION, "]", "Expected ] to close index expression");

    return exp;
}

//parses the actual stream of expression tokens
Node* parseExpression(Parser* p, int rbp) {
    Token tok = CUR(p);
    OperatorType op_type = lookupOperator(tok); // maps Token -> OperatorType

    Node* left = NULL;                                                 //no allocation because pointer gets overriden
    if (op_type != OPERATOR_NONE && rules[op_type].operator.nud != NULL) //consume operator
    {
        skip(p, "advancing past prefix/nud token");
        left = rules[op_type].operator.nud(p);
    } else if (IS_LITERAL(p) || IS_IDENTIFIER(p)) //consume literal/variable
    {
        left = nudPrimary(p);
    } else //unexpected token error
    {
        parserError("Unexpected token '%s' at position %d, expected start of expression", tok.text, p->pos);
    }

    while (1) {
        Token next_tok = CUR(p);
        OperatorType next_op = lookupOperator(next_tok);                        //peek next token
        int lbp = (next_op != OPERATOR_NONE) ? rules[next_op].operator.lbp : 0; //lbp is 0 if next is identifier/literal

        if (lbp <= rbp)
            break;

        skip(p, "advancing past infix/led token");
        left = rules[next_op].operator.led(p, left); //call led
    }

    return left;
}

Node* handleForeverStatement(Parser* p) {
    Node* forever_stmt = malloc(sizeof(Node));
    forever_stmt->type = NODE_FOREVER_STMT;
    consume(p, TOKEN_KEYWORD, "forever", "expected forever statement");
    forever_stmt->forever_loop.logic = handleStatement(p);
    return forever_stmt;
}

Node* handleSingleArgLoop(Parser* p) {
    Node* loop_stmt = malloc(sizeof(Node));
    if (IS_KEYWORD(p, "while")) {
        loop_stmt->type = NODE_WHILE_STMT;
        consume(p, TOKEN_KEYWORD, "while", "expected while statement");
    } else if (IS_KEYWORD(p, "until")) {
        loop_stmt->type = NODE_UNTIL_STMT;
        consume(p, TOKEN_KEYWORD, "until", "expected until statement");
    } else if (IS_KEYWORD(p, "repeat")) {
        loop_stmt->type = NODE_REPEAT_STMT;
        consume(p, TOKEN_KEYWORD, "repeat", "expected repeat statement");
    }
    consume(p, TOKEN_PUNCTUATION, "(", "Expected ( after loop keyword");
    loop_stmt->single_arg_loop.expr = parseExpression(p, 0);
    consume(p, TOKEN_PUNCTUATION, ")", "Expected ) after loop condition");
    loop_stmt->single_arg_loop.logic = handleStatement(p);
    return loop_stmt;
}

Node* handleIterateLoop(Parser* p) {
    Node* iter_stmt = malloc(sizeof(Node));
    iter_stmt->type = NODE_ITERATE_STMT;
    consume(p, TOKEN_KEYWORD, "iterate", "expected iterate statement");
    consume(p, TOKEN_PUNCTUATION, "(", "Expected ( after iterate");
    iter_stmt->iterate_loop.source = parseExpression(p, 0);
    consume(p, TOKEN_PUNCTUATION, ";", "Expected ; after source expression");
    iter_stmt->iterate_loop.item = handleVarDecl(p);
    consume(p, TOKEN_PUNCTUATION, ")", "Expected ) after loop condition");
    iter_stmt->iterate_loop.logic = handleStatement(p);
    return iter_stmt;
}

Node* handleDoLoop(Parser *p) {
    Node* do_stmt = malloc(sizeof(Node));
    consume(p, TOKEN_KEYWORD, "do", "expected do statement");
    do_stmt->do_loop.logic = handleStatement(p);
    if(IS_KEYWORD(p, "while")) {
        consume(p, TOKEN_KEYWORD, "while", "expected while statement"); 
        do_stmt->type = NODE_DO_WHILE_STMT;
    }
    else if (IS_KEYWORD(p, "until")){
        consume(p, TOKEN_KEYWORD, "until", "expected until statement"); 
        do_stmt->type = NODE_DO_UNTIL_STMT;
    }
    else parserError("unrecognized keyword after do statement at position %d", p->pos);
    consume(p, TOKEN_PUNCTUATION, "(", "Expected ( after do statement keyword");
    do_stmt->do_loop.expr = parseExpression(p, 0);
    consume(p, TOKEN_PUNCTUATION, ")", "Expected ) after loop condition");
    consume(p, TOKEN_PUNCTUATION, ";", "Expected ; to terminate statement!");
    return do_stmt;
}

Node* handleWhenStatement(Parser* p) {
    Node* statement = malloc(sizeof(Node));
    if (IS_KEYWORD(p, "when")) {
        statement->type = NODE_WHEN_STMT;
        consume(p, TOKEN_KEYWORD, "when", "expected when statement");
        consume(p, TOKEN_PUNCTUATION, "(", "Expected ( after when"); //consume paren so ) fallthrough means it will stop on final ), leaving it as cur
        statement->when_stmt.expr = parseExpression(p, 0);
        consume(p, TOKEN_PUNCTUATION, ")", "Expected ) after if condition");
        statement->when_stmt.logic = handleStatement(p);
        if (IS_KEYWORD(p, "else") || IS_KEYWORD(p, "elif"))
            statement->when_stmt.else_stmt = handleWhenStatement(p);
        else
            statement->when_stmt.else_stmt = NULL;
    } else if (IS_KEYWORD(p, "else")) {
        statement->type = NODE_ELSE_STMT;
        consume(p, TOKEN_KEYWORD, "else", "expected else statement");
        statement->else_stmt.logic = handleStatement(p);
    } else if (IS_KEYWORD(p, "elif")) {
        statement->type = NODE_ELSE_STMT;
        consume(p, TOKEN_KEYWORD, "elif", "expected elif statement");
        Node* elif_stmt = malloc(sizeof(Node));
        elif_stmt->type = NODE_WHEN_STMT;
        statement->else_stmt.logic = elif_stmt; //wrap when statement in else statement
        consume(p, TOKEN_PUNCTUATION, "(", "Expected ( after elif");
        elif_stmt->when_stmt.expr = parseExpression(p, 0);
        consume(p, TOKEN_PUNCTUATION, ")", "Expected ) after elif condition");
        elif_stmt->when_stmt.logic = handleStatement(p);
        if (IS_KEYWORD(p, "else") || IS_KEYWORD(p, "elif"))
            elif_stmt->when_stmt.else_stmt = handleWhenStatement(p);
        else
            elif_stmt->when_stmt.else_stmt = NULL;
    }
    return statement;
}

Node* handleIfStatement(Parser* p) {
    Node* statement = malloc(sizeof(Node));
    if (IS_KEYWORD(p, "if")) {
        statement->type = NODE_IF_STMT;
        consume(p, TOKEN_KEYWORD, "if", "expected if statement");
        consume(p, TOKEN_PUNCTUATION, "(", "Expected ( after if"); //consume paren so ) fallthrough means it will stop on final ), leaving it as cur
        statement->if_stmt.expr = parseExpression(p, 0);
        consume(p, TOKEN_PUNCTUATION, ")", "Expected ) after if condition");
        statement->if_stmt.logic = handleStatement(p);
        if (IS_KEYWORD(p, "else") || IS_KEYWORD(p, "elif"))
            statement->if_stmt.else_stmt = handleIfStatement(p);
        else
            statement->if_stmt.else_stmt = NULL;
    } else if (IS_KEYWORD(p, "else")) {
        statement->type = NODE_ELSE_STMT;
        consume(p, TOKEN_KEYWORD, "else", "expected else statement");
        statement->else_stmt.logic = handleStatement(p);
    } else if (IS_KEYWORD(p, "elif")) //another terrible repetitive kludge because i am too lazy to refactor
    {
        statement->type = NODE_ELSE_STMT;
        consume(p, TOKEN_KEYWORD, "elif", "expected elif statement");
        Node* elif_stmt = malloc(sizeof(Node));
        elif_stmt->type = NODE_IF_STMT;
        statement->else_stmt.logic = elif_stmt; //wrap if statement in else statement
        consume(p, TOKEN_PUNCTUATION, "(", "Expected ( after elif");
        elif_stmt->if_stmt.expr = parseExpression(p, 0);
        consume(p, TOKEN_PUNCTUATION, ")", "Expected ) after elif condition");
        elif_stmt->if_stmt.logic = handleStatement(p);
        if (IS_KEYWORD(p, "else") || IS_KEYWORD(p, "elif"))
            elif_stmt->if_stmt.else_stmt = handleIfStatement(p);
        else
            elif_stmt->if_stmt.else_stmt = NULL;
    }
    return statement;
}

Node* handleReturnStatement(Parser* p) {
    consume(p, TOKEN_KEYWORD, "return", "Expected return statement!");
    Node* stmt = malloc(sizeof(Node));
    stmt->type = NODE_RETURN_STMT;
    stmt->return_stmt.expr = parseExpression(p, 0);
    consume(p, TOKEN_PUNCTUATION, ";", "Expected ; to terminate statement!");
    return stmt;
}

Node* handleExpressionStatement(Parser* p) {
    Node* expression_statement = malloc(sizeof(Node));
    expression_statement->type = NODE_EXPR_STMT;
    expression_statement->expr_stmt.expr = parseExpression(p, 0);
    consume(p, TOKEN_PUNCTUATION, ";", "Expected ; to terminate statement!");
    return expression_statement;
}

//starts on whatever is after ; or }
Node* handleStatement(Parser* p) {
    //logic for determining kind of statement will go here
    //STUB, not all statements implemented.
    if (IS_OPEN_BRACE(p))
        return handleCodeBlock(p);
    if (IS_KEYWORD(p, "when"))
        return handleWhenStatement(p);
    if (IS_KEYWORD(p, "if"))
        return handleIfStatement(p);
    if (IS_KEYWORD(p, "else") || IS_KEYWORD(p, "elif"))
        parserError("Expected opening if statement to else or elif at position %d", p->pos);
    if (IS_KEYWORD(p, "forever"))
        return handleForeverStatement(p);
    if (IS_KEYWORD(p, "until") || IS_KEYWORD(p, "while") || IS_KEYWORD(p, "repeat"))
        return handleSingleArgLoop(p);
    if (IS_KEYWORD(p, "do"))
        return handleDoLoop(p);
    if (IS_KEYWORD(p, "iterate"))
        return handleIterateLoop(p);
    if (IS_KEYWORD(p, "typedef"))
        return handleTypedef(p);
    if (IS_KEYWORD(p, "object")) {
        Node* decl = handleObjectDecl(p, true);
        consume(p, TOKEN_PUNCTUATION, ";", "Expected ; after object declaration");
        return decl;
    }
    if (isTypeName(p, &CUR(p))) {
        Node* decl = handleVarDecl(p);
        consume(p, TOKEN_PUNCTUATION, ";", "Expected ; after variable declaration");
        return decl;
    }
    if (IS_KEYWORD(p, "return"))
        return handleReturnStatement(p);
    return handleExpressionStatement(p);
}

//starts on opening {, ends after closing }
Node* handleCodeBlock(Parser* p) {
    Node* block = malloc(sizeof(Node));
    block->type = NODE_BLOCK;

    int statement_cap = 8;
    Node** statements = malloc(sizeof(Node*) * statement_cap);
    int statement_count = 0;

    consume(p, TOKEN_PUNCTUATION, "{", "Expected opening { for code block");

    while (!IS_CLOSE_BRACE(p)) {
        if (p->pos >= p->token_count || CUR(p).type == TOKEN_EOF)
            parserError("Expected } before end of file");

        if (statement_count >= statement_cap) {
            statement_cap *= 2;
            statements = realloc(statements, sizeof(Node*) * statement_cap);
        }
        statements[statement_count++] = handleStatement(p);
    }

    consume(p, TOKEN_PUNCTUATION, "}", "Expected } at end of code block");

    block->block.statements = statements;
    block->block.count = statement_count;
    return block;
}

char* handleImport(Parser* p) {
    consume(p, TOKEN_KEYWORD, "import", "Expected import");
    Token name_tok = consume(p, TOKEN_IDENTIFIER, NULL, "Expected import name");
    consume(p, TOKEN_PUNCTUATION, ";", "Expected ; after import name");
    return name_tok.text;
}

Node* handleGlobal(Parser* p) {
    Node* global = handleVarDecl(p);
    consume(p, TOKEN_PUNCTUATION, ";", "Expected ; after global declaration");
    return global;
}

//starts on function name, return type already parsed
Node* handleFunction(Parser* p, Node* return_type) {
    Node* function = malloc(sizeof(Node));
    function->type = NODE_FUNCTION_DECL;
    function->function_decl.return_type = return_type;
    function->function_decl.name = CUR(p).text;

    int param_count;
    Node** params = handleArgList(p, true, &param_count);
    function->function_decl.param_count = param_count;
    function->function_decl.params = params;

    if (IS_OPEN_BRACE(p)) {
        function->function_decl.body = handleCodeBlock(p);
    } else {
        consume(p, TOKEN_PUNCTUATION, ";", "Expected { or ; after function declaration");
        function->function_decl.body = NULL;
    }

    return function;
}

Node* parse(Parser* p) {
    int import_count = 0;
    int function_count = 0;
    int global_count = 0;

    int import_cap = 8, function_cap = 8, global_cap = 8;
    char** imports = malloc(sizeof(char*) * import_cap);
    Node** functions = malloc(sizeof(Node*) * function_cap);
    Node** globals = malloc(sizeof(Node*) * global_cap);

    while (CUR(p).type != TOKEN_EOF) {
        if (IS_KEYWORD(p, "import")) {
            if (import_count >= import_cap) {
                import_cap *= 2;
                imports = realloc(imports, sizeof(char*) * import_cap);
            }
            imports[import_count++] = handleImport(p);
            continue;
        }

        if (IS_KEYWORD(p, "typedef")) {
            if (global_count >= global_cap) {
                global_cap *= 2;
                globals = realloc(globals, sizeof(Node*) * global_cap);
            }
            globals[global_count++] = handleTypedef(p);
            continue;
        }

        if (IS_KEYWORD(p, "object")) {
            Node* object = handleObjectDecl(p, true);
            consume(p, TOKEN_PUNCTUATION, ";", "Expected ; after object declaration");
            if (global_count >= global_cap) {
                global_cap *= 2;
                globals = realloc(globals, sizeof(Node*) * global_cap);
            }
            globals[global_count++] = object;
            continue;
        }

        if (IS_IDENTIFIER(p) && NEXT_PUNCT(p, "(")) //untyped function, implicit void return
        {
            if (function_count >= function_cap) {
                function_cap *= 2;
                functions = realloc(functions, sizeof(Node*) * function_cap);
            }
            functions[function_count++] = handleFunction(p, makePrimitiveType(PRIMITIVE_VOID));
            continue;
        }

        if (isTypeName(p, &CUR(p))) {
            int start = p->pos;
            Node* type = parseType(p);

            if (IS_IDENTIFIER(p) && NEXT_PUNCT(p, "(")) {
                if (function_count >= function_cap) {
                    function_cap *= 2;
                    functions = realloc(functions, sizeof(Node*) * function_cap);
                }
                functions[function_count++] = handleFunction(p, type);
                continue;
            }

            p->pos = start;
            if (global_count >= global_cap) {
                global_cap *= 2;
                globals = realloc(globals, sizeof(Node*) * global_cap);
            }
            globals[global_count++] = handleGlobal(p);
            continue;
        }

        parserError("Unexpected token '%s' at top level, position %d", CUR(p).text, p->pos);
    }

    Node* program_node = malloc(sizeof(Node));
    *program_node = (Node){
        .type = NODE_PROGRAM,
        .program = {
            .imports = imports,
            .import_count = import_count,
            .functions = functions,
            .function_count = function_count,
            .globals = globals,
            .global_count = global_count,
        }};
    return program_node;
}