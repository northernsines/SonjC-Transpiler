build:
	clang main.c lexer.c parser.c parser_utils.c io_utils.c print.c cli.c -o transpiler -Wall -Werror -Wno-return-type