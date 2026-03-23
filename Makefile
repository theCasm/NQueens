SRCS := src/bitset.c src/constraint.c
OBJS := $(foreach w, $(SRCS), $(w:src/%.c=bin/%.o))
DEV_OBJS := $(foreach w, $(SRCS), $(w:src/%.c=bin/%.dev.o))

CFLAGS = -Wall -Wpedantic

prod: $(OBJS)
	gcc $(CFLAGS) -O3 $^ -o bin/constraint

dev: $(DEV_OBJS)
	gcc $(CFLAGS) -ggdb -Og $^ -o bin/constraint


bin/%.o: src/%.c
	mkdir -p bin
	gcc $(CFLAGS) -O3 -c $< -o $@

bin/%.dev.o: src/%.c
	mkdir -p bin
	gcc $(CFLAGS) -ggdb -Og -c $< -o $@