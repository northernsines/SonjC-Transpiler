#include "cli.h"
#include "io_utils.h"
#include "parser.h"
#include "print.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
SonjC Main Entry Point
Written Aug 2026
Raw entry point and pipeline: lex, parse, and run the debug output stages.
*/

int main(int argc, char* argv[]) {
    CliOptions opts = parseArgs(argc, argv);

    char* source = readFile(opts.file_path);

    size_t tokens = 0;

    Lexer lexer = {.source = source, .pos = 0, .line = 1, .col = 1, .char_filled = false, .char_mode = false, .string_mode = false};

    size_t capacity = 64; //start with a capacity of 64 tokens

    Token* token_array = malloc(sizeof(Token) * capacity);
    if (token_array == NULL) {
        fprintf(stderr, "malloc returned null for initial token array allocation \n");
        exit(EXIT_FAILURE);
    }

    Token tok;
    while (1) {
        tok = nextToken(&lexer);

        if (tokens >= capacity) {
            capacity *= 2;
            token_array = realloc(token_array, sizeof(Token) * capacity);
        }

        if (!(tok.type == TOKEN_PUNCTUATION && tok.length == 1 && (tok.text[0] == ' ' || tok.text[0] == '\n'))) {
            tok.text = strdup(tok.text);
            token_array[tokens] = tok;
            tokens++;
        }

        if (tok.type == TOKEN_EOF)
            break; // escape on EOF iteration
    }
    if (opts.print_tok)
        printTokenStream(token_array, tokens);

    initRules();
    Parser parser = {.tokens = token_array, .token_count = tokens, .pos = 0, .def_types = NULL, .def_type_count = 0};
    Node* ast = parse(&parser);
    if (opts.print_ast)
        printNode(ast);

    free(source);
    return 0;
}