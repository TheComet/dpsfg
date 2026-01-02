#include "csfg/util/bm.h"
#include "csfg/util/mem.h"
#include <stddef.h>
#include <string.h>

/* -------------------------------------------------------------------------- */
void bm_init(struct bm** bm)
{
    *bm = NULL;
}

/* -------------------------------------------------------------------------- */
void bm_deinit(struct bm* bm)
{
    mem_free(bm);
}

/* -------------------------------------------------------------------------- */
struct bm* bm_create(int num_bits)
{
    int        header = offsetof(struct bm, data);
    int        count = (num_bits + 63) / 64;
    struct bm* bm = mem_alloc(header + count * sizeof(bm->data[0]));
    if (bm == NULL)
        return NULL;
    memset(bm->data, 0, count * sizeof(bm->data[0]));
    bm->count = count;
    return bm;
}

/* -------------------------------------------------------------------------- */
int bm_realloc(struct bm** bm, int num_bits)
{
    struct bm* new_bm;
    int        header = offsetof(struct bm, data);
    int        old_count = bm_count(*bm);
    int        new_count = (num_bits + 63) / 64;

    if (new_count <= old_count)
        return 0;

    new_bm = mem_realloc(*bm, header + new_count * sizeof((*bm)->data[0]));
    if (new_bm == NULL)
        return -1;
    new_bm->count = new_count;
    *bm = new_bm;

    return 0;
}

/* -------------------------------------------------------------------------- */
void bm_set_all(struct bm* bm)
{
    if (bm != NULL)
        memset(bm->data, 0xFF, bm->count * sizeof(bm->data[0]));
}

/* -------------------------------------------------------------------------- */
void bm_reset_all(struct bm* bm)
{
    if (bm != NULL)
        memset(bm->data, 0, bm->count * sizeof(bm->data[0]));
}
