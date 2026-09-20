#ifndef CLI_H
#define CLI_H
#include <stdbool.h>

/*
SonjC CLI Options Header
Written Sep 2026
Defines command line option parsing for the transpiler driver.
*/

typedef struct {
    bool printTok;
    bool printAst;
    const char *filePath;
} CliOptions;

CliOptions parseArgs(int argc, char *argv[]);

#endif