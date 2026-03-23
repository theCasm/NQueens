/*
 * Author: Aidan Sigmund Sebastian Undheim
 * Copyright (c) 2026
 *
 * A simple bitset library in C.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "bitset.h"

#define min(a,b) ((a) < (b) ? (a) : (b))

// NOTE: assumes len != 0 ever. 0 breaks stuff! AGGGHHHh
// anything left over in the last chunk must be set to zero.
// the buf is little endian.

#define bs_num_chunks(bs) (1 + (((bs).len - 1) / 64))

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

static inline int chunk_cardinality(uint64_t domain)
{
    int ans = 0;
    while (domain) {
        ans += 1;
        domain &= domain - 1lu;    
    }
    return ans;
}

bitset bs_new_universe(size_t len)
{
    bitset ans = {len};
    ans.buf = calloc(bs_num_chunks(ans), sizeof(*ans.buf));
    for (int i = 0; i < bs_num_chunks(ans) - 1; i++) {
        ans.buf[i] = ~(0lu);
    }
    ans.buf[bs_num_chunks(ans) - 1lu] = ~(0lu) >> (64lu - (len - 64*bs_num_chunks(ans)));
    return ans;
}

bitset bs_new_empty(size_t len)
{
    bitset ans = {len};
    ans.buf = calloc(bs_num_chunks(ans), sizeof(*ans.buf));
    return ans;
}

void bs_emptyify(bitset bs)
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
    for (int i = 0; i < min(bs_num_chunks(dest), bs_num_chunks(src)); i++) {
        dest.buf[i] |= src.buf[i];
    }
}

void bs_and(bitset dest, bitset src)
{
    for (int i = 0; i < min(bs_num_chunks(dest), bs_num_chunks(src)); i++) {
        dest.buf[i] &= src.buf[i];
    }
}

void bs_set(bitset dest, bitset src)
{
    for (int i = 0; i < min(bs_num_chunks(dest), bs_num_chunks(src)); i++) {
        dest.buf[i] = src.buf[i];
    }
}

int bs_eq(bitset dest, bitset src)
{
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

int bs_cardinality(bitset bs)
{
    int ans = 0;
    for (int i = 0; i < bs_num_chunks(bs); i++) {
        ans += chunk_cardinality(bs.buf[i]);
    }
    return ans;
}

unsigned int bs_empty(bitset bs)
{
    for (int i = 0; i < bs_num_chunks(bs); i++) {
        if (bs.buf[i] != 0) return 0;
    }
    return 1;
}

int bs_is_singleton(bitset bs)
{
    int found = 0;
    for (int i = 0; i < bs_num_chunks(bs); i++) {
        if (bs.buf[i] != 0) {
            if (!is_chunk_singleton(bs.buf[i])) {
                return 0;
            } else {
                if (found == 1) return 0;
                else found = 1;
            }
        }
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
    bitset temp = bs_new_empty(bs.len);
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
    bs_emptyify(dest);
    for (int i = 0; i < bs_num_chunks(src); i++) {
        if (src.buf[i] != 0lu) {
            dest.buf[i] = src.buf[i] - (src.buf[i] & (src.buf[i] - 1lu));
            src.buf[i] -= dest.buf[i];
            break;
        }
    }
}

bitset bs_copy(bitset orig)
{
    bitset ans = bs_new_empty(orig.len);
    bs_set(ans, orig);
    return ans;
}