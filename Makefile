SRCS := src/bitset.c src/constraint.c

CFLAGS = -Wall -Wpedantic

prod: $(SRCS)
	mkdir -p bin
	gcc $(CFLAGS) -O3 src/constraint.c -o bin/constraint

dev: $(SRCS)
	mkdir -p bin
	gcc $(CFLAGS) -ggdb -Og src/constraint.c -o bin/constraint