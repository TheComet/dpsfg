#include "csfg/numeric/mat.h"

/* -------------------------------------------------------------------------- */
int csfg_mat_lu_decomposition(
    struct csfg_mat** Lp, struct csfg_mat** Up, const struct csfg_mat* input)
{
    struct csfg_mat *L, *U;
    int              i, j;

    if (csfg_mat_realloc(Lp, csfg_mat_rows(input), csfg_mat_cols(input)) != 0 ||
        csfg_mat_realloc(Up, csfg_mat_rows(input), csfg_mat_cols(input)) != 0)
        return -1;
    L = *Lp;
    U = *Up;

    csfg_mat_identity(L);
    csfg_mat_copy(U, input);

    for (i = 0; i != csfg_mat_rows(U); ++i)
        for (j = i + 1; j < csfg_mat_rows(U); ++j)
        {
            struct csfg_complex c0 = *csfg_mat_get(U, i, i);
            struct csfg_complex c1 = *csfg_mat_get(U, j, i);
            struct csfg_complex factor = csfg_complex_div(c1, c0);
            csfg_mat_r1_sub_r2_mul_s_store_r1(U, j, i, factor);
            *csfg_mat_get(L, j, i) = factor;
        }

    return 0;
}
