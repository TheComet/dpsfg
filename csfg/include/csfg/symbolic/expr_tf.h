#pragma once

#include "csfg/util/strview.h"
#include "csfg/util/vec.h"

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

/*!
 * @brief Given an expression in "standard polynomial form", this function
 * returns an array containing the root nodes of each coefficient of the
 * numerator and denominator polynomials. @see csfg_expr_to_standard_tf().
 *
 * Assumed Input:
 *   a0 + a1*s^1 + a2*s^2 + ...
 * Output:
 *   coeff = [a0, a1, a2, ...]
 *
 * The coefficients are required to build a transfer_function object, which is
 * required for calculating impulse/step responses, frequency responses,
 * pole-zero diagrams, etc.
 * @param[inout] pool The expression to extract the coefficients from. All
 * occurrences of the input variable will be removed from the expression, and
 * it will be split into multiple sub-expressions. The old "root" will become
 * invalid after this function returns.
 * @param[in] variable This is the dependent variable, usually "s".
 * @return Returns -1 on failure, 0 on succes.
 */
int csfg_expr_extract_poly_coeff(
    struct csfg_expr_pool** pool,
    int                     expr,
    struct strview          variable,
    struct csfg_expr_vec**  coeff);

int csfg_expr_apply_limit(struct csfg_expr_pool* pool);
