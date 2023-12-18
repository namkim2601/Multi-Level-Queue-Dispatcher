CFLAGS=-O0 -Werror=vla -std=gnu11 -g -fsanitize=address -pthread -lm

all: process mlqd

process: sigtrap.c
	gcc -o process sigtrap.c

mlqd: mab.c pcb.c mlqd.c
	gcc $(CFLAGS) -o mlqd mab.c pcb.c mlqd.c

clean:
	rm -f process mlqd

.PHONY: all clean