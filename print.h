#ifndef PRINT_H
#define PRINT_H
#include "parser.h"

/*
SonjC Print Header
Written Sep 2026
Declares debug printing for token streams and the AST.
*/

void printTokenStream(Token *tokens, int count);
const char *tokentypeToString(TokenType type);
const char *operatorTypeToString(OperatorType op);
void printNode(Node *node, int depth);

#endif