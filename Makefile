CC ?= gcc
CFLAGS ?= -O3
CFLAGS += -fopenmp

all: life

life: life.c
	$(CC) $(CFLAGS) life.c -o life

clean:
	rm -f life

.PHONY: all clean