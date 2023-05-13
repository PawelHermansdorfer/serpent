CFLAGS=-Wall -Wextra -Werror -pedantic -g -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable
CC=gcc

all: serpent

serpent: serpent.c utils.c utils.h
	$(CC) $(CFLAGS) serpent.c -o serpent

.PHONY: clean
clean: serpent
	rm ./serpent
