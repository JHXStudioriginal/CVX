# Makefile for CVX shell
CC = gcc
CFLAGS = -Wall -Wextra -lm
SRC = src/main.c src/config.c src/prompt.c src/exec.c src/signals.c src/linenoise.c
OBJ = $(SRC:.c=.o)
OUT = cvx

.PHONY: all clean

all: $(OUT)

$(OUT): $(OBJ)
	$(CC) $(OBJ) -o $(OUT) $(CFLAGS)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm -f $(OUT) $(OBJ)
