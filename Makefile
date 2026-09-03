build:
	clang main.c lexer.c parser.c -o transpiler -Wall -Werror -Wno-return-type