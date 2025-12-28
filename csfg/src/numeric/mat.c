#include "csfg/numeric/mat.h"
#include "csfg/util/mem.h"
#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

VEC_DEFINE(csfg_mat_reorder, int8_t, 8)

/* -------------------------------------------------------------------------- */
void csfg_mat_init(struct csfg_mat** mat)
{
    *mat = NULL;
}

/* -------------------------------------------------------------------------- */
void csfg_mat_deinit(struct csfg_mat* mat)
{
    if (mat)
        mem_free(mat);
}

/* -------------------------------------------------------------------------- */
int csfg_mat_realloc(struct csfg_mat** mat, int rows, int cols)
{
    int              header = offsetof(struct csfg_mat, data);
    int              data = rows * cols * sizeof((*mat)->data[0]);
    struct csfg_mat* new_mat = mem_realloc(*mat, header + data);
    if (new_mat == NULL)
        return -1;
    new_mat->rows = rows;
    new_mat->cols = cols;
    *mat = new_mat;
    return 0;
}

/* -------------------------------------------------------------------------- */
void csfg_mat_identity(struct csfg_mat* mat)
{
    int i;
    csfg_mat_zero(mat);
    for (i = 0; i != csfg_mat_rows(mat) && i != csfg_mat_cols(mat); ++i)
        *csfg_mat_get(mat, i, i) = csfg_complex(1, 0);
}

/* -------------------------------------------------------------------------- */
void csfg_mat_zero(struct csfg_mat* mat)
{
    int i;
    for (i = 0; i != csfg_mat_rows(mat) * csfg_mat_cols(mat); ++i)
        mat->data[i] = csfg_complex(0, 0);
}

/* -------------------------------------------------------------------------- */
void csfg_mat_copy(struct csfg_mat* out, const struct csfg_mat* in)
{
    CSFG_DEBUG_ASSERT(csfg_mat_cols(out) == csfg_mat_cols(in));
    CSFG_DEBUG_ASSERT(csfg_mat_rows(out) == csfg_mat_rows(in));
    memcpy(
        out->data,
        in->data,
        csfg_mat_rows(out) * csfg_mat_cols(out) * sizeof(out->data[0]));
}

/* -------------------------------------------------------------------------- */
void csfg_mat_set_real(struct csfg_mat* mat, ...)
{
    va_list ap;
    int     r, c;

    va_start(ap, mat);
    for (r = 0; r != csfg_mat_rows(mat); ++r)
        for (c = 0; c != csfg_mat_cols(mat); ++c)
            *csfg_mat_get(mat, r, c) = csfg_complex(va_arg(ap, double), 0);
    va_end(ap);
}

/* -------------------------------------------------------------------------- */
int csfg_mat_reorder_identity(
    struct csfg_mat_reorder** reorder, const struct csfg_mat* mat)
{
    int i;

    if (csfg_mat_reorder_realloc(reorder, csfg_mat_rows(mat)) != 0)
        return -1;

    for (i = 0; i != csfg_mat_rows(mat); ++i)
        csfg_mat_reorder_push_no_realloc(*reorder, i);

    return 0;
}

/* -------------------------------------------------------------------------- */
static void reorder_visit(struct csfg_mat_reorder* reorder, int row)
{
    *vec_get(reorder, row) = -1 - *vec_get(reorder, row);
}
static int reorder_is_visited(const struct csfg_mat_reorder* reorder, int row)
{
    return *vec_get(reorder, row) < 0;
}
static void reorder_unvisit_all(struct csfg_mat_reorder* reorder)
{
    int i;
    for (i = 0; i < vec_count(reorder); ++i)
    {
        int next = *vec_get(reorder, i);
        if (next < 0)
            *vec_get(reorder, i) = -1 - next;
    }
}
static struct csfg_complex apply_reorder_recurse(
    struct csfg_mat*         mat,
    struct csfg_mat_reorder* reorder,
    int                      start_row,
    int                      row,
    int                      col)
{
    int                 next_row;
    struct csfg_complex save;

    next_row = *vec_get(reorder, row);
    if (next_row != start_row)
        save = apply_reorder_recurse(mat, reorder, start_row, next_row, col);
    else
        save = *csfg_mat_get(mat, next_row, col);

    *csfg_mat_get(mat, next_row, col) = *csfg_mat_get(mat, row, col);
    reorder_visit(reorder, row);
    return save;
}
void csfg_mat_apply_reorder(
    struct csfg_mat* mat, struct csfg_mat_reorder* reorder)
{
    int i, c;
    CSFG_DEBUG_ASSERT(vec_count(reorder) == csfg_mat_rows(mat));

    for (c = 0; c != csfg_mat_cols(mat); ++c)
    {
        for (i = 0; i < vec_count(reorder); ++i)
        {
            struct csfg_complex last;
            int                 next = *vec_get(reorder, i);
            if (next == i || reorder_is_visited(reorder, i))
                continue;
            last = apply_reorder_recurse(mat, reorder, i, next, c);
            *csfg_mat_get(mat, next, c) = last;
            reorder_visit(reorder, i);
        }
        reorder_unvisit_all(reorder);
    }
}

/* -------------------------------------------------------------------------- */
void csfg_mat_r1_sub_r2_mul_s_store_r1(
    struct csfg_mat*    mat,
    int                 row_minuend,
    int                 row_subtrahend,
    struct csfg_complex scalar)
{
    int c;
    for (c = 0; c != csfg_mat_cols(mat); ++c)
        *csfg_mat_get(mat, row_minuend, c) = csfg_complex_sub(
            *csfg_mat_get(mat, row_minuend, c),
            csfg_complex_mul(scalar, *csfg_mat_get(mat, row_subtrahend, c)));
}

/* -------------------------------------------------------------------------- */
void csfg_mat_rotate_rows_in_range(
    struct csfg_mat* mat, struct csfg_mat_reorder* reorder, int r1, int r2)
{
    int r, c, tmp_idx;
    CSFG_DEBUG_ASSERT(vec_count(reorder) == csfg_mat_rows(mat));
    CSFG_DEBUG_ASSERT(r1 >= 0);
    CSFG_DEBUG_ASSERT(r1 < csfg_mat_rows(mat));
    CSFG_DEBUG_ASSERT(r2 >= 1);
    CSFG_DEBUG_ASSERT(r2 <= csfg_mat_rows(mat));

    for (c = 0; c != csfg_mat_cols(mat); ++c)
    {
        struct csfg_complex tmp = *csfg_mat_get(mat, r1, c);
        for (r = r1 + 1; r < r2; ++r)
            *csfg_mat_get(mat, r - 1, c) = *csfg_mat_get(mat, r, c);
        *csfg_mat_get(mat, r - 1, c) = tmp;
    }

    tmp_idx = *vec_get(reorder, r1);
    for (r = r1 + 1; r < r2; ++r)
        *vec_get(reorder, r - 1) = *vec_get(reorder, r);
    *vec_get(reorder, r - 1) = tmp_idx;
}

/* -------------------------------------------------------------------------- */
void csfg_mat_swap_rows(
    struct csfg_mat* mat, struct csfg_mat_reorder* reorder, int r1, int r2)
{
    int c, tmp_idx;
    CSFG_DEBUG_ASSERT(vec_count(reorder) == csfg_mat_rows(mat));
    CSFG_DEBUG_ASSERT(r1 >= 0);
    CSFG_DEBUG_ASSERT(r1 < csfg_mat_rows(mat));
    CSFG_DEBUG_ASSERT(r2 >= 0);
    CSFG_DEBUG_ASSERT(r2 < csfg_mat_rows(mat));

    for (c = 0; c != csfg_mat_cols(mat); ++c)
    {
        struct csfg_complex tmp = *csfg_mat_get(mat, r1, c);
        *csfg_mat_get(mat, r1, c) = *csfg_mat_get(mat, r2, c);
        *csfg_mat_get(mat, r2, c) = tmp;
    }

    tmp_idx = *vec_get(reorder, r2);
    *vec_get(reorder, r2) = *vec_get(reorder, r1);
    *vec_get(reorder, r1) = tmp_idx;
}

/* -------------------------------------------------------------------------- */
int csfg_mat_mul(
    struct csfg_mat** out, const struct csfg_mat* a, const struct csfg_mat* b)
{
    int r, c, i;

    CSFG_DEBUG_ASSERT(csfg_mat_cols(a) == csfg_mat_rows(b));
    if (csfg_mat_realloc(out, csfg_mat_cols(a), csfg_mat_rows(b)) != 0)
        return -1;

    csfg_mat_zero(*out);
    for (r = 0; r != csfg_mat_rows(*out); ++r)
        for (c = 0; c != csfg_mat_cols(*out); ++c)
            for (i = 0; i != csfg_mat_rows(*out); ++i)
            {
                struct csfg_complex* entry = csfg_mat_get(*out, r, c);
                struct csfg_complex  product = csfg_complex_mul(
                    *csfg_mat_get(a, r, i), *csfg_mat_get(b, i, c));
                *entry = csfg_complex_add(*entry, product);
            }

    return 0;
}
