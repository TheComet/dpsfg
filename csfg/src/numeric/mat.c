#include "csfg/numeric/mat.h"
#include "csfg/util/mem.h"
#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

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
    for (i = 0; i != csfg_mat_rows(mat) || i != csfg_mat_cols(mat); ++i)
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
