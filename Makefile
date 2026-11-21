CC = clang
SRCS = $(wildcard *.c)
CFLAGS = -Wall -Werror -Wno-return-type
BUILDOPT = -O2
DEBUGOPT = -O0
DEBUG = -g
OUT = transpiler
RFLAGS = -printast -printtok
TARGET = test.sc

 .PHONY: build debug clean run

build:
	$(CC) $(SRCS) $(CFLAGS) $(BUILDOPT) -o $(OUT)
debug:
	$(CC) $(SRCS) $(CFLAGS) $(DEBUGOPT) -o $(OUT) $(DEBUG)
clean:
	rm -f $(OUT)
	rm -rf $(OUT).dSYM
run:
	./$(OUT) $(TARGET) $(RFLAGS)