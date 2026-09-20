#include "parser.h"
#include "io_utils.h"
#include "cli.h"
#include "print.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
SonjC Main Entry Point
Written Aug 2026
Raw entry point and pipeline: lex, parse, and run the debug output stages.
*/

int main(int argc, char *argv[]) {
    CliOptions opts = parseArgs(argc, argv);

    char *source = read_file(opts.filePath);

    size_t tokens = 0;

    Lexer lexer = { .source = source, .pos = 0, .line = 1, .col = 1, .charFilled = false, .charMode = false, .stringMode = false};

    size_t capacity = 64; //start with a capacity of 64 tokens

    Token *tokenArray = malloc(sizeof(Token) * capacity); 
    if (tokenArray == NULL)
    {
        fprintf(stderr, "malloc returned null for initial token array allocation \n"); 
        exit(EXIT_FAILURE);
    }

    Token tok;
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
    if (opts.printTok) printTokenStream(tokenArray, tokens);

    initRules();
    Parser parser = { .tokens = tokenArray, .tokenCount = tokens, .pos = 0 };
    Node *ast = parse(&parser);
    if (opts.printAst) printNode(ast, 0);

    free(source);
    return 0;
}