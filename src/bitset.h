#ifndef BITSET_H
#define BITSET_H

typedef struct {
    size_t len;
    uint64_t *buf;
} bitset;

bitset bs_new_universe(size_t len);
bitset bs_new_empty(size_t len);
void bs_emptyify(bitset bs);
void bs_free(bitset bs);
void bs_or(bitset dest, bitset src);
void bs_and(bitset dest, bitset src);
void bs_set(bitset dest, bitset src);
int bs_eq(bitset dest, bitset src);
void bs_not(bitset dest);
void bs_shl(bitset dest, unsigned int amnt);
void bs_shr(bitset dest, unsigned int amnt);
int bs_cardinality(bitset bs);
unsigned int bs_empty(bitset bs);
int bs_is_singleton(bitset bs);
unsigned int bs_get_min(bitset bs);
void bs_print(bitset bs);
void bs_pop_min_singleton(bitset dest, bitset src);
bitset bs_copy(bitset orig);


#endif