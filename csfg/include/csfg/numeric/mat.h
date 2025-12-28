#pragma once

#include "csfg/numeric/complex.h"
#include "csfg/util/vec.h"

struct csfg_mat
{
    int                 rows, cols;
    struct csfg_complex data[1];
};

VEC_DECLARE(csfg_mat_reorder, int8_t, 8)

void csfg_mat_init(struct csfg_mat** mat);
void csfg_mat_deinit(struct csfg_mat* mat);
int  csfg_mat_realloc(struct csfg_mat** mat, int rows, int cols);
void csfg_mat_identity(struct csfg_mat* mat);
void csfg_mat_zero(struct csfg_mat* mat);
void csfg_mat_copy(struct csfg_mat* out, const struct csfg_mat* in);
void csfg_mat_set_real(struct csfg_mat* mat, ...);
int  csfg_mat_mul(
     struct csfg_mat** out, const struct csfg_mat* a, const struct csfg_mat* b);

/* Row operations ----------------------------------------------------------- */
/* r2 - s*r1 -> r2 */
int csfg_mat_reorder_identity(
    struct csfg_mat_reorder** reorder, const struct csfg_mat* mat);
void csfg_mat_apply_reorder(
    struct csfg_mat* mat, struct csfg_mat_reorder* reorder);
void csfg_mat_r1_sub_r2_mul_s_store_r1(
    struct csfg_mat*    mat,
    int                 row_minuend,
    int                 row_subtrahend,
    struct csfg_complex scalar);
/*! r1 inclusive, r2 exclusive */
void csfg_mat_rotate_rows_in_range(
    struct csfg_mat* mat, struct csfg_mat_reorder* reorder, int r1, int r2);
void csfg_mat_swap_rows(
    struct csfg_mat* mat, struct csfg_mat_reorder* reorder, int r1, int r2);

int csfg_mat_lu_decomposition(
    struct csfg_mat**         L,
    struct csfg_mat**         U,
    struct csfg_mat_reorder** reorder,
    const struct csfg_mat*    input);

/*!
 * \brief Solves a linear system of equations from an LU-decomposed matrix
 * (\see csfg_mat_lu_decomposition).
 *
 *   [m00 m01 m02] [out0]   [in0]
 *   [m10 m11 m12] [out1] = [in1]
 *   [m20 m21 m22] [out2]   [in2]
 *
 * where
 *
 *   [m00 m01 m02]   [l00 l01 l02] [u00 u01 u02]
 *   [m10 m11 m12] = [l10 l11 l12] [u10 u11 u12]
 *   [m20 m21 m22]   [l20 l21 l22] [u20 u21 u22]
 *
 * \param[out] out The resulting column-vector is written to this matrix.
 * \param[inout] in The caller should fill this column-vector with the list of
 * input values.
 * \warning The vector is modified internally to hold intermediate results.
 */
void csfg_mat_solve_linear_lu(
    struct csfg_mat*               out,
    struct csfg_mat*               in,
    const struct csfg_mat*         L,
    const struct csfg_mat*         U,
    const struct csfg_mat_reorder* reorder);

#define csfg_mat_rows(mat)          ((mat) ? (mat)->rows : 0)
#define csfg_mat_cols(mat)          ((mat) ? (mat)->cols : 0)
#define csfg_mat_get(mat, row, col) ((mat)->data + (mat)->cols * (row) + (col))
#define csfg_mat_begin(mat)         ((mat) ? (mat)->data : NULL)
#define csfg_mat_end(mat)                                                      \
    ((mat) ? (mat)->data + (mat)->rows * (mat)->cols : NULL)
#define csfg_mat_for_each(mat, var)                                            \
    for (var = csfg_mat_begin(mat); var != csfg_mat_end(mat); var++)
