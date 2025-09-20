# Makefile for CVX shell
CC = gcc
CFLAGS = -Wall -Wextra -lm
SRC = src/cvx.c src/linenoise.c
OUT = cvx

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(SRC) -o $(OUT) $(CFLAGS)

clean:
	rm -f $(OUT)

