/*
 * Author: Aidan Sigmund Sebastian Undheim
 * Copyright (c) 2026
 * 
 * TODO: only works up to heights of 64
 * TODO: replace (in enforce...) the temp = ...; if temp == mask; ...; stuff w/ a better way
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "bitset.h"
#include "bitset.c" // I know this is sketchy as hell, but I lose 30% performance if I use multiple files since the optimizer gets freaked out :(

#define FAIL 0
#define CHOSE 1
#define DEFAULT 2

#define debug(str, domains) do {if (1) {printf("%d" str, stack_index); for (int i = 0; i < min(height, 4); i++) bs_print(domains[i]); putchar('\n');}} while (0)
#define min(a, b) ((a) < (b) ? (a) : (b))

static uint64_t height;
static int *board;

#define DSTACK_H (height + 1)

static bitset temp, temp2, mask;
static bitset (**domain_stack);
static int stack_index;

void init()
{
    board = malloc(sizeof(*board) * height);
    temp = bs_new_empty(2*height);
    temp2 = bs_new_empty(2*height);
    mask = bs_new_empty(2*height);
    domain_stack = malloc(sizeof(*domain_stack) * DSTACK_H);
    for (int i = 0; i < DSTACK_H; i++) {
        domain_stack[i] = malloc(sizeof(**domain_stack) * height);
        for (int j = 0; j < height; j++) {
            domain_stack[i][j] = bs_new_universe(height);
        }    
    }
    stack_index = 0;
}

void cleanup()
{
    free(board);
    free(temp.buf);
    free(temp2.buf);
    free(mask.buf);
    for (int i = 0; i < DSTACK_H; i++) {
        for (int j = 0; j < height; j++) {
            free(domain_stack[i][j].buf);
        }    
        free(domain_stack[i]);
    }
    free(domain_stack);
}

int enforce_distinct(bitset *domains)
{
    int chose = 0;
    bs_emptyify(mask);
    for (int i = 0; i < height; i++) {
        if (bs_is_singleton(domains[i])) {
            bs_set(temp, mask);
            bs_or(mask, domains[i]);
            if (bs_eq(temp, mask)) return FAIL;
        }
    }
    bs_not(mask);
    for (int i = 0; i < height; i++) {
        if (bs_is_singleton(domains[i])) continue;
        bs_and(domains[i], mask);
        if (bs_empty(domains[i])) return FAIL;
        if (bs_is_singleton(domains[i])) {
            chose |= 1;
        }
    }
    if (chose) return CHOSE;
    return DEFAULT;
}

// TODO: is there something better than this 3-pass way?
int enforce_nodiagnol(bitset *domains)
{
    int chose = 0;
    bs_emptyify(mask);
    int i;
    for (i = 0; i < height; i++) {
        if (bs_is_singleton(domains[i])) {
            bs_set(temp, mask);
            bs_or(mask, domains[i]);
            if (bs_eq(temp, mask)) return FAIL;
        }
        bs_shl(mask, 1);
    }
    bs_shr(mask, 1);
    bs_not(mask);
    bs_emptyify(temp2);
    for (i -= 1; i >= 0; i--) {
        if (bs_is_singleton(domains[i])) {
            bs_set(temp, temp2);
            bs_or(temp2, domains[i]);
            if (bs_eq(temp, temp2)) return FAIL;
        } else {
            bs_and(domains[i], mask);
            if (bs_empty(domains[i])) return FAIL;
            else if (bs_is_singleton(domains[i])) chose |= 1;
        }
        bs_shl(temp2, 1);
        bs_shr(mask, 1);
    }
    bs_shr(temp2, 1);
    bs_not(temp2);
    for (i = 0; i < height; i++) {
        if (!bs_is_singleton(domains[i])) {
            bs_and(domains[i], temp2);
            if (bs_empty(domains[i])) return FAIL;
            else if (bs_is_singleton(domains[i])) chose |= 1;
        }
        bs_shr(temp2, 1);
    }
    if (chose) return CHOSE;
    return DEFAULT;
}

int choose_next_domain(bitset *domains)
{
    int minI = -1, minV = -1;
    for (int i = 0; i < height; i++) {
        if (!bs_is_singleton(domains[i])) {
            if (minI == -1 || bs_cardinality(domains[i]) < minV) {
                minI = i;
                minV = bs_cardinality(domains[i]);
            }
        }
    }
    return minI;
}

// TODO: make iterative
int solve()
{
    bitset *domains = domain_stack[stack_index];
    //debug("SOLVE: ", domains);
    int res, chose;
    do {
        chose = 0;
        res = enforce_distinct(domains);
        //debug("AFTER DIST: ", domains);
        if (res == FAIL) return FAIL;
        else if (res == CHOSE) chose |= 1;
        res = enforce_nodiagnol(domains);
        //debug("AFTER DIAG: ", domains);
        if (res == FAIL) return FAIL;
        else if (res == CHOSE) chose |= 1;
    } while (chose);
    int next_domain = choose_next_domain(domains);
    if (next_domain == -1) {
        //debug("sucess? ", domains);
        return 1;
    };
    //printf("----------------------TRYING-------------- %d\n", next_domain);
    while (!bs_empty(domains[next_domain])) {
        stack_index += 1;
        for (int i = 0; i < height; i++) bs_set(domain_stack[stack_index][i], domains[i]);        
        bs_pop_min_singleton(domain_stack[stack_index][next_domain], domains[next_domain]);     
        //debug("COPY: ", domain_stack[stack_index]);
        int res = solve();
        if (res) {
            return 1;
        }
        stack_index -= 1;
    }
    //printf("----------------------FAIL---------------- %d\n", next_domain);
    return FAIL;
}

static inline int diag(int curr, int try)
{
    for (int i = 0; i < curr; i++) {
        if (abs(board[i] - try) == abs(i - curr)) return 1;
    }
    return 0;
}

int verify()
{
    for (int i = 0; i < height; i++) {
        if (diag(i, board[i])) {
            fputs("FAIL DIAG\n", stderr);
            return 0;
        }
    }
    int arr[height];
    for (int i = 0; i < height; i++) {
        arr[i] = 0;
    }
    for (int i = 0; i < height; i++) {
        if (arr[board[i]] != 0) {
            fprintf(stderr, "FAIL DIST %d\n", board[i]);
            return 0;
        }
        arr[board[i]] = 1;
    }
    return 1;
}

void tryH(int h)
{
    height = h;
    init();
    printf("N=%ld: ", height);
    if (!solve()) {
        puts("No solution.");
        return;
    }
    for (int i = 0; i < height; i++) {
        board[i] = bs_get_min(domain_stack[stack_index][i]);
    }
    if (!verify()) {
        fputs("Exitting due to failure...\n", stderr);
        exit(1);
    }
    for (int i = 0; i < height; i++) printf("%d, ", board[i]);
    putchar('\n');
    cleanup();
}

int main(int argc, char *argv[])
{
    if (argc >= 2) {
        tryH(atoi(argv[1]));
        return 0;
    }
    for (int i = 1; ; i++) {
        tryH(i);
    } 
    free(board);
    return 0;
}