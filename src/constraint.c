/*
 * Author: Aidan Sigmund Sebastian Undheim
 * Copyright (c) 2026
 * 
 * TODO: only works up to heights of 64
 * TODO: replace (in enforce...) the temp = ... if temp = mask stuff w/ a better way
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define FAIL 0
#define CHOSE 1
#define DEFAULT 2

#define debug(str, domains) do {if (1) {printf("%d" str, stack_index); for (int i = 0; i < height; i++) bs_print(domains[i]); putchar('\n');}} while (0)
#define min(a, b) ((a) < (b) ? (a) : (b))

static uint64_t height;
static int *board;

/*
 * BEGIN BITSET LIB (put in other file eventually)
*/

// NOTE: assumes len != 0 ever. 0 breaks stuff! AGGGHHHh
// anything left over in the last chunk must be set to zero.
// the buf is little endian.
typedef struct {
    size_t len;
    uint64_t *buf;
} bitset; 

#define bs_num_chunks(bs) (1 + (((bs).len - 1) / 64))

bitset new_bs_universe(size_t len)
{
    bitset ans = {len};
    ans.buf = calloc(bs_num_chunks(ans), sizeof(*ans.buf));
    for (int i = 0; i < bs_num_chunks(ans) - 1; i++) {
        ans.buf[i] = ~(0lu);
    }
    ans.buf[bs_num_chunks(ans) - 1lu] = ~(0lu) >> (64lu - (len - 64*bs_num_chunks(ans)));
    return ans;
}

bitset new_bs_empty(size_t len)
{
    bitset ans = {len};
    ans.buf = calloc(bs_num_chunks(ans), sizeof(*ans.buf));
    return ans;
}

void set_bs_empty(bitset bs)
{
    for (int i = 0; i < bs_num_chunks(bs); i++) {
        bs.buf[i] = 0lu;
    }
}

void bs_free(bitset bs)
{
    free(bs.buf);
}

void bs_or(bitset dest, bitset src)
{
    // TODO: expected behaviour if lengths unequal
    // alternatively, assume equal bcs they are (in this application) for optimization?
    for (int i = 0; i < min(bs_num_chunks(dest), bs_num_chunks(src)); i++) {
        dest.buf[i] |= src.buf[i];
    }
}

void bs_and(bitset dest, bitset src)
{
    // TODO: expected behaviour if lengths unequal
    // alternatively, assume equal bcs they are (in this application) for optimization?
    for (int i = 0; i < min(bs_num_chunks(dest), bs_num_chunks(src)); i++) {
        dest.buf[i] &= src.buf[i];
    }
}

void bs_set(bitset dest, bitset src)
{
    // TODO: expected behaviour if lengths unequal
    // alternatively, assume equal bcs they are (in this application) for optimization?
    for (int i = 0; i < min(bs_num_chunks(dest), bs_num_chunks(src)); i++) {
        dest.buf[i] = src.buf[i];
    }
}

int bs_eq(bitset dest, bitset src)
{
    // TODO: expected behaviour if lengths unequal
    // alternatively, assume equal bcs they are (in this application) for optimization?
    for (int i = 0; i < min(bs_num_chunks(dest), bs_num_chunks(src)); i++) {
        if (dest.buf[i] != src.buf[i]) return 0;
    }
    return 1;
}

void bs_not(bitset dest)
{
    int i;
    for (i = 0; i < bs_num_chunks(dest) - 1; i++) {
        dest.buf[i] = ~dest.buf[i];
    }
    dest.buf[i] = (~dest.buf[i]) & (~(0lu) >> (64*bs_num_chunks(dest) - dest.len));
}

void bs_shl(bitset dest, unsigned int amnt)
{
    int start = amnt / 64;
    uint64_t rem = amnt - 64*start;
    int i;
    for (i = bs_num_chunks(dest) - 1lu; i > start; i--) {
        dest.buf[i] = dest.buf[i - start] << rem | (dest.buf[i - start - 1] >> (64-rem));
    }
    dest.buf[i] = dest.buf[i - start] << rem;
    for (i -= 1; i >= 0; i--) dest.buf[i] = 0lu;
}

void bs_shr(bitset dest, unsigned int amnt)
{
    int chunks = amnt / 64;
    uint64_t rem = amnt - 64*chunks;
    int i;
    for (i = 0; i < bs_num_chunks(dest) - chunks - 1; i++) {
        dest.buf[i] = dest.buf[i + chunks] >> rem | (dest.buf[i + chunks + 1] << (64-rem));
    }
    dest.buf[i] = dest.buf[i + chunks] >> rem;
    for (i += 1; i < bs_num_chunks(dest); i++) dest.buf[i] = 0lu;
}

static inline int is_chunk_singleton(uint64_t domain)
{
    return (domain & (domain - 1lu)) == 0lu;
}

// todo: optimize
static inline unsigned int perfect_log2(uint64_t x)
{
    unsigned int ans = 0;
    if (x & 0xFFFFFFFF00000000lu) ans |= 32lu;
    if (x & 0xFFFF0000FFFF0000lu) ans |= 16lu;
    if (x & 0xFF00FF00FF00FF00lu) ans |= 8lu;
    if (x & 0xF0F0F0F0F0F0F0F0lu) ans |= 4lu;
    if (x & 0xCCCCCCCCCCCCCCCClu) ans |= 2lu;
    if (x & 0xAAAAAAAAAAAAAAAAlu) ans |= 1lu;
    return ans;
}

// assumes there is a min
static inline unsigned int get_chunk_min(uint64_t domain)
{
    return perfect_log2(domain - (domain & (domain - 1lu)));
}

int chunk_cardinality(uint64_t domain)
{
    int ans = 0;
    while (domain) {
        ans += 1;
        domain &= domain - 1lu;    
    }
    return ans;
}

int bs_cardinality(bitset bs)
{
    int ans = 0;
    for (int i = 0; i < bs_num_chunks(bs); i++) {
        ans += chunk_cardinality(bs.buf[i]);
    }
    return ans;
}

int bs_is_singleton(bitset bs)
{
    for (int i = 0; i < bs_num_chunks(bs); i++) {
        if (!is_chunk_singleton(bs.buf[i])) return 0;
    }
    return 1;
}

// assumes there is a min
unsigned int bs_get_min(bitset bs)
{
    unsigned int ans = 0;
    for (int i = 0; i < bs_num_chunks(bs); i++) {
        if (bs.buf[i] != 0) {
            ans += get_chunk_min(bs.buf[i]);
            break;
        }
        ans += 64;
    }
    return ans;
}

void bs_print(bitset bs)
{   
    bitset temp = new_bs_empty(bs.len);
    bs_set(temp, bs);
    for (int i = 0; i < bs.len; i++) {
        if (temp.buf[0] & 1lu) printf("%d, ", i);
        bs_shr(temp, 1lu);
    }
    putchar('\n');
    bs_free(temp);
}

void bs_pop_min_singleton(bitset dest, bitset src)
{
    set_bs_empty(dest);
    for (int i = 0; i < bs_num_chunks(src); i++) {
        if (src.buf[i] != 0lu) {
            dest.buf[i] = src.buf[i] - (src.buf[i] & (src.buf[i] - 1lu));
            src.buf[i] -= dest.buf[i];
            break;
        }
    }
}

unsigned int bs_empty(bitset bs)
{
    for (int i = 0; i < bs_num_chunks(bs); i++) {
        if (bs.buf[i] != 0) return 0;
    }
    return 1;
}

bitset bs_copy(bitset orig)
{
    bitset ans = new_bs_empty(orig.len);
    bs_set(ans, orig);
    return ans;
}


/*
 * END BITSET LIB
*/

static bitset temp, temp2, mask;
static bitset (**domain_stack);
static int stack_index;

void init_enforce()
{
    temp = new_bs_empty(2*height);
    temp2 = new_bs_empty(2*height);
    mask = new_bs_empty(2*height);
    domain_stack = malloc(sizeof(*domain_stack) * (height + 1));
    for (int i = 0; i < height + 1; i++) {
        domain_stack[i] = malloc(sizeof(**domain_stack) * height);
        for (int j = 0; j < height; j++) {
            domain_stack[i][j] = new_bs_universe(height);
        }    
    }
    stack_index = 0;
}

void cleanup_enforce()
{
    free(temp.buf);
    free(temp2.buf);
    free(mask.buf);
    for (int i = 0; i < height + 1; i++) {
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
    set_bs_empty(mask);
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
    set_bs_empty(mask);
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
    set_bs_empty(temp2);
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
    /*for (uint64_t i = 0; i < height; i++) {
        if (bs_is_singleton(domains[i])) {
            for (uint64_t j = 0; j < height; j++) {
                unsigned int dist = abs(i - j);
                bs_set(temp, domains[j]);
                bs_set(mask, domains[i]);
                bs_shl(mask, dist*2);
                bs_or(mask, domains[i]);
                bs_shr(mask, dist);
                bs_not(mask);
                if (j != i) {
                    bs_and(domains[j], mask);
                }
                if (bs_empty(domains[j])) return FAIL;
                if (!bs_is_singleton(temp) && bs_is_singleton(domains[j])) chose |= 1;
            }
        }
    }*/
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
            //return i;
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

void verify()
{
    for (int i = 0; i < height; i++) {
        if (diag(i, board[i])) {
            fputs("FAIL DIAG\n", stderr);
        }
    }
    int arr[height];
    for (int i = 0; i < height; i++) {
        arr[i] = 0;
    }
    for (int i = 0; i < height; i++) {
        if (arr[board[i]] != 0) fprintf(stderr, "FAIL DIST %d\n", board[i]);
        arr[board[i]] = 1;
    }
}

// TODO: move init/cleanup fns appropriately
void tryH(char h)
{
    height = h;
    board = realloc(board, sizeof(*board) * height);
    init_enforce();
    solve();
    for (int i = 0; i < height; i++) {
        board[i] = bs_get_min(domain_stack[stack_index][i]);
    }
    printf("N=%d: ", h);
    for (int i = 0; i < height; i++) printf("%d, ", board[i]);
    putchar('\n');
    verify();
    cleanup_enforce();
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
    /*bitset temp = new_bs_empty(128);
    temp.buf[0] = 1;
    bs_shl(temp, 65);
    printf("%d\n", bs_get_min(temp));*/
}