#include "csfg/numeric/mat.h"

/* -------------------------------------------------------------------------- */
static int
reorder_rows_if_necessary(struct csfg_mat* M, struct csfg_mat_reorder* reorder)
{
    int i, r;
    for (i = 0; i != csfg_mat_rows(M); ++i)
    {
        if (fabs(csfg_mat_get(M, i, i)->real) > 1e-6)
            continue;

        for (r = i + 1; r < csfg_mat_rows(M); ++r)
            if (fabs(csfg_mat_get(M, r, i)->real) > 1e-6)
            {
                csfg_mat_swap_rows(M, reorder, i, r);
                break;
            }
        if (r >= csfg_mat_rows(M))
            return -1;
    }
    return 0;
}

/* -------------------------------------------------------------------------- */
int csfg_mat_lu_decomposition(
    struct csfg_mat**         Lp,
    struct csfg_mat**         Up,
    struct csfg_mat_reorder** reorder,
    const struct csfg_mat*    input)
{
    struct csfg_mat *L, *U;
    int              i, j;

    if (csfg_mat_realloc(Lp, csfg_mat_rows(input), csfg_mat_cols(input)) != 0 ||
        csfg_mat_realloc(Up, csfg_mat_rows(input), csfg_mat_cols(input)) != 0 ||
        csfg_mat_reorder_identity(reorder, input) != 0)
    {
        return -1;
    }
    L = *Lp;
    U = *Up;

    csfg_mat_identity(L);
    csfg_mat_copy(U, input);

    if (reorder_rows_if_necessary(U, *reorder) != 0)
        return -1;

    for (i = 0; i != csfg_mat_rows(U); ++i)
        for (j = i + 1; j < csfg_mat_rows(U); ++j)
        {
            struct csfg_complex c0, c1, factor;
            c0 = *csfg_mat_get(U, i, i);
            c1 = *csfg_mat_get(U, j, i);
            factor = csfg_complex_div(c1, c0);
            csfg_mat_r1_sub_r2_mul_s_store_r1(U, j, i, factor);
            *csfg_mat_get(L, j, i) = factor;
        }

    return 0;
}
