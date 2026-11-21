#include "cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
SonjC CLI Options
Written Sep 2026
Parses command line flags, prints help and usage diagnostics, and exits on
invalid invocations.
*/

static void printHelp(const char* prog) {
    printf("SonjC Transpiler\n\n");
    printf("usage: %s [-printtok] [-printast] [-help] <file.sc>\n\n", prog);
    printf("flags:\n");
    printf("  -printtok    print the lexer token stream\n");
    printf("  -printast    print the parsed AST\n");
    printf("  -help        show this help message\n");
}

CliOptions parseArgs(int argc, char* argv[]) {
    bool print_help_flag = false;
    for (int i = 1; i < argc; i++)
        if (strcmp(argv[i], "-help") == 0)
            print_help_flag = true;

    if (print_help_flag) {
        printHelp(argv[0]);
        exit(EXIT_SUCCESS);
    }

    if (argc < 2) {
        fprintf(stderr, "usage: %s [-flags] <file.sc>\n\nUse the \"-help\" flag to show a list of flags.\n\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    CliOptions opts = {.print_tok = false, .print_ast = false, .file_path = NULL};

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-printtok") == 0)
            opts.print_tok = true;
        else if (strcmp(argv[i], "-printast") == 0)
            opts.print_ast = true;
        else
            opts.file_path = argv[i];
    }

    if (!opts.file_path) {
        fprintf(stderr, "no input file provided\n");
        exit(EXIT_FAILURE);
    }

    return opts;
}