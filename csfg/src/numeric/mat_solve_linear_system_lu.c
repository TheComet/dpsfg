#include "csfg/config.h"
#include "csfg/numeric/mat.h"
#include <assert.h>

/* -------------------------------------------------------------------------- */
void csfg_mat_solve_linear_lu(
    struct csfg_mat*       out,
    struct csfg_mat*       in,
    const struct csfg_mat* L,
    const struct csfg_mat* U)
{
    int c, r;

    CSFG_DEBUG_ASSERT(csfg_mat_cols(in) == 1);
    CSFG_DEBUG_ASSERT(csfg_mat_cols(out) == 1);
    CSFG_DEBUG_ASSERT(csfg_mat_rows(in) == csfg_mat_rows(L));
    CSFG_DEBUG_ASSERT(csfg_mat_rows(out) == csfg_mat_rows(L));
    CSFG_DEBUG_ASSERT(csfg_mat_rows(L) == csfg_mat_cols(L));
    CSFG_DEBUG_ASSERT(csfg_mat_rows(U) == csfg_mat_cols(U));
    CSFG_DEBUG_ASSERT(csfg_mat_rows(L) == csfg_mat_rows(U));
    CSFG_DEBUG_ASSERT(csfg_mat_cols(L) == csfg_mat_cols(U));

    /* Solve out=L*in */
    csfg_mat_zero(out);
    for (r = 0; r != csfg_mat_rows(L); ++r)
    {
        *csfg_mat_get(out, r, 0) = *csfg_mat_get(in, r, 0);

        /* clang-format off */
        for (c = 0; c != r ; ++c)
            *csfg_mat_get(out, r, 0) = csfg_complex_sub(
                *csfg_mat_get(out, r, 0), csfg_complex_mul(
                    *csfg_mat_get(L, r, c),
                    *csfg_mat_get(out, c, 0)
                )
            );

        *csfg_mat_get(out, r, 0) = csfg_complex_div(
            *csfg_mat_get(out, r, 0),
            *csfg_mat_get(L, r, r));
        /* clang-format on */
    }

    csfg_mat_copy(in, out);

    /* Solve out=U*in */
    csfg_mat_zero(out);
    for (r = csfg_mat_rows(U) - 1; r >= 0; --r)
    {
        *csfg_mat_get(out, r, 0) = *csfg_mat_get(in, r, 0);

        /* clang-format off */
        for (c = csfg_mat_cols(U) - 1; c != r ; --c)
            *csfg_mat_get(out, r, 0) = csfg_complex_sub(
                *csfg_mat_get(out, r, 0), csfg_complex_mul(
                    *csfg_mat_get(U, r, c),
                    *csfg_mat_get(out, c, 0)
                )
            );

        *csfg_mat_get(out, r, 0) = csfg_complex_div(
            *csfg_mat_get(out, r, 0),
            *csfg_mat_get(U, r, r));
        /* clang-format on */
    }
}
