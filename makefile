CC=gcc
CFLAGS=-lm

all:
	$(CC) $(CFLAGS) -o Digger digger.h digger.c load.c plant.c
clean:
	rm -rf Diggers
