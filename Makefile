CC = gcc
CFLAGS = -Wall -pedantic -g #-fsanitize=address

all: shell

shell: shell.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	-rm -f shell
