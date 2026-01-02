#pragma once

#include <stdint.h>

struct bm
{
    int      count;
    uint64_t data[1];
};

void        bm_init(struct bm** bm);
void        bm_deinit(struct bm* bm);
struct bm*  bm_create(int num_bits);
int         bm_realloc(struct bm** bm, int num_bits);
void        bm_set_all(struct bm* bm);
void        bm_reset_all(struct bm* bm);
static void bm_set(struct bm* bm, int bit)
{
    int      idx = bit / 64;
    uint64_t mask = 1UL << (bit & 0x3F);
    bm->data[idx] |= mask;
}
static int bm_set_and_test(struct bm* bm, int bit)
{
    int      idx = bit / 64;
    uint64_t mask = 1UL << (bit & 0x3F);
    int      was_set = (bm->data[idx] & mask) != 0UL;
    bm->data[idx] |= mask;
    return was_set;
}
static void bm_reset(struct bm* bm, int bit)
{
    int      idx = bit / 64;
    uint64_t mask = 1UL << (bit & 0x3F);
    bm->data[idx] &= ~mask;
}
static int bm_test(struct bm* bm, int bit)
{
    int      idx = bit / 64;
    uint64_t mask = 1UL << (bit & 0x3F);
    return (bm->data[idx] & mask) != 0UL;
}

#define bm_count(bm) ((bm) ? (bm)->count : 0)
