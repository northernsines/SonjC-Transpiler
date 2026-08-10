#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Token *tokenArray;
size_t tokens;


char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "couldn't open %s\n", path); //invalid file
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f); //get file size
    fseek(f, 0, SEEK_SET);

    char *buffer = malloc(size + 1); //allocate mem
    if (buffer == NULL)
    {
        fprintf(stderr, "malloc returned null for file content buffer allocation \n"); 
        exit(EXIT_FAILURE);
    }
    size_t bytesRead = fread(buffer, 1, size, f);

    if (bytesRead != (size_t)size)
    {
        fprintf(stderr, "failed to read %s\n", path);
        fclose(f);
        free(buffer);
        exit(EXIT_FAILURE);
    }
    buffer[size] = '\0';   // null-terminate to treat like normal C string

    fclose(f);
    return buffer;
}

void printNode(Node *node, int depth)
{
    if (!node) return;

    for (int i = 0; i < depth; i++) printf("  "); // indent per depth

    switch (node->type)
    {
        case NODE_LITERAL:
            printf("Literal: %s\n", node->literal.value);
            break;

        case NODE_VAR:
            printf("Var: %s\n", node->var.name);
            break;

        case NODE_VAR_DECL:
            printf("VarDecl: %s = %s\n", node->varDecl.name, node->varDecl.value);
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
            printf("FunctionDecl: %s -> %s\n",
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

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <file.sc>\n", argv[0]); //not enough args
        exit(EXIT_FAILURE);
    }
    tokens = 0;

    char *source = read_file(argv[1]); //store file to source
    Lexer lexer = { .source = source, .pos = 0, .line = 1, .col = 1, .charFilled = false, .charMode = false, .stringMode = false};

    size_t capacity = 64; //start with a capacity of 64 tokens

    Token *tokenArray = malloc(sizeof(Token) * capacity); 
    if (tokenArray == NULL)
    {
        fprintf(stderr, "malloc returned null for initial token array allocation \n"); 
        exit(EXIT_FAILURE);
    }

    Token tok = nextToken(&lexer);
    while (1)
    {
        tok = nextToken(&lexer);

        if (tokens >= capacity)
        {
            capacity *= 2;
            tokenArray = realloc(tokenArray, sizeof(Token) * capacity);
        }

        if (!(tok.type == TOKEN_PUNCTUATION && tok.length == 1 && (tok.text[0] == ' ' || tok.text[0] == '\n')))
        {
            tok.text = strdup(tok.text);
            tokenArray[tokens] = tok;
            tokens++;
        }

        if (tok.type == TOKEN_EOF) break; // escape on EOF iteration
    }
    for (int i = 0; i < tokens; i++)
    {
        printf("[%02d] type=%s text=%s", i, tokentypeToString(tokenArray[i].type), tokenArray[i].text);
    }

    Parser parser = { .tokens = tokenArray, .tokenCount = tokens, .pos = 0 };
    Node *ast = parse(&parser);
    printNode(ast, 0);

    free(source);
    return 0;
}