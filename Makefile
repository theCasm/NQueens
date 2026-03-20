prod: src/constraint.c
	mkdir -p bin
	gcc -Wall -O3 src/constraint.c -o bin/constraint

dev: src/constraint.c
	mkdir -p bin
	gcc -Wall -Wpedantic -ggdb -O0 src/constraint.c -o bin/constraint
