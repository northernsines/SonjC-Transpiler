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

static void printHelp(const char *prog)
{
    printf("SonjC Transpiler\n\n");
    printf("usage: %s [-printtok] [-printast] [-help] <file.sc>\n\n", prog);
    printf("flags:\n");
    printf("  -printtok    print the lexer token stream\n");
    printf("  -printast    print the parsed AST\n");
    printf("  -help        show this help message\n");
}

CliOptions parseArgs(int argc, char *argv[])
{
    bool printHelpFlag = false;
    for (int i = 1; i < argc; i++)
        if (strcmp(argv[i], "-help") == 0) printHelpFlag = true;

    if (printHelpFlag) {
        printHelp(argv[0]);
        exit(EXIT_SUCCESS);
    }

    if (argc < 2) {
        fprintf(stderr, "usage: %s [-printtok] [-printast] [-help] <file.sc>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    CliOptions opts = { .printTok = false, .printAst = false, .filePath = NULL };

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-printtok") == 0) opts.printTok = true;
        else if (strcmp(argv[i], "-printast") == 0) opts.printAst = true;
        else opts.filePath = argv[i];
    }

    if (!opts.filePath) {
        fprintf(stderr, "no input file provided\n");
        exit(EXIT_FAILURE);
    }

    return opts;
}