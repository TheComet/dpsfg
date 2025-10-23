#pragma once

#include "csfg/symbolic/poly_expr.h"
#include "csfg/util/strview.h"

struct csfg_expr_pool;
struct csfg_var_table;

struct csfg_tf_expr
{
    struct csfg_poly_expr* num;
    struct csfg_poly_expr* den;
};

static void csfg_tf_expr_init(struct csfg_tf_expr* r)
{
    csfg_poly_expr_init(&r->num);
    csfg_poly_expr_init(&r->den);
}

static void csfg_tf_expr_deinit(struct csfg_tf_expr* r)
{
    csfg_poly_expr_deinit(r->num);
    csfg_poly_expr_deinit(r->den);
}

static void csfg_tf_expr_clear(struct csfg_tf_expr* r)
{
    csfg_poly_expr_clear(r->num);
    csfg_poly_expr_clear(r->den);
}

int csfg_expr_to_rational(
    const struct csfg_expr_pool* in_pool,
    int                          in_expr,
    struct strview               main_var,
    struct csfg_expr_pool**      tf_pool,
    struct csfg_tf_expr*         tf);

/*!
 * @brief Calculates lim_{variable->oo} of an expression and returns it as a
 * rational function. In the case of divergence, zero and infinity remain
 * symbolic. I.e. no floating point infinity or NaN values are set in the
 * result.
 *
 * The second version, _limits(), accepts a variable table instead of a single
 * variable. In this case, each "oo" entry in the table will be applied in
 * succession.
 *
 * If successful, the result will be one of:
 *   1) oo/1
 *   2) -oo/1
 *   3) 1/oo
 *   4) -1/oo
 *   5) 0/0
 *   6) a/b, where a and b are expressions
 *
 * Returns -1 on failure, 0 on success.
 */
int csfg_expr_to_rational_limit(
    const struct csfg_expr_pool* in_pool,
    int                          in_expr,
    struct strview               variable,
    struct csfg_expr_pool**      tf_pool,
    struct csfg_tf_expr*         tf);
int csfg_expr_to_rational_limits(
    const struct csfg_expr_pool* in_pool,
    int                          in_expr,
    const struct csfg_var_table* vt,
    struct csfg_expr_pool**      tf_pool,
    struct csfg_tf_expr*         tf);

int csfg_rational_to_expr(
    const struct csfg_tf_expr*   tf,
    const struct csfg_expr_pool* tf_pool,
    struct csfg_expr_pool**      expr_pool);

VEC_DECLARE(csfg_expr_vec, int, 8)

struct csfg_expr_pool;
struct csfg_expr_vec;

/*!
 * @brief Attempts to re-arrange the expression into the standard "Transfer
 * function form":
 *
 *          b0 + b1*s^1 + b2*s^2 + ...
 *   T(s) = --------------------------
 *          a0 + a1*s^1 + a2*s^2 + ...
 *
 * There are all sorts of reasons why this might fail, but if it succeeds, it
 * means each polynomial coefficient can be extracted using @see
 * csfg_expr_extract_poly_coeff(), which allows you to change variables and
 * re-evaluate the new coefficient values without needing to do this
 * transformation each time.
 *
 * If this function doesn't succeed, which will be the case if any term that
 * contains the specified variable (usually "s") is combined with an operation
 * that is *not* mul/div/add/sub with a variable operand (such as the
 * expression s^a) -- or in other words, if the operation containing the
 * specified variable is not reducible to polynomial form -- the expressions
 * will be in a modified state different from the initial condition, but
 * mathematically equivalent. You may want to combine the numerator and
 * denominator using @see csfg_expr_div() back into a single expression to
 * recover. In this case, no expressions exist for the polynomial coefficients
 * and the transfer function will have to perform this manipulation every time
 * a variable changes.
 *
 * It is worth noting that DPSFGs will always produce expressions reducible
 * to polynomial expressions. User input may not.
 *
 * @param[inout] num_pool The input expression to transform. This is typically
 * obtained from @see csfg_expr_parse() or similar. On success, the numerator
 * of the transfer function will be written back.
 * @param[inout] num_root The root node of the input expression. The root node
 * will change as transformations are applied.
 * @param[out] den_pool The denominator expression will be stored into this
 * parameter. Should initially be set to an empty pool. @see
 * csfg_expr_pool_init().
 * @param[out] den_root The root node of the denominator expression is written
 * to this parameter.
 * @param[in] variable This is the dependent variable, usually "s".
 * @return Returns -1 on failure, 0 on succes.
 */
int csfg_expr_to_standard_tf(
    struct csfg_expr_pool** num_pool,
    int*                    num_root,
    struct csfg_expr_pool** den_pool,
    int*                    den_root);

/*!
 * Moves all common reciprocs from numerator to denominator or vice-versa, such
 * that no more reciprocs exist. Combined with other operations, this will
 * normalize the fraction.
 */
int csfg_expr_rebalance_fraction(
    struct csfg_expr_pool** num_pool,
    int                     num_root,
    struct csfg_expr_pool** den_pool,
    int                     den_root);
