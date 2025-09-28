#pragma once

#include <stdarg.h>

struct csfg_expr_pool;
typedef int (*csfg_expr_pass_func)(struct csfg_expr_pool**);

int csfg_expr_op_run(struct csfg_expr_pool** pool, ...);
int csfg_expr_op_runv(struct csfg_expr_pool** pool, va_list ap);

int csfg_expr_op_run_pass(
    struct csfg_expr_pool** pool, csfg_expr_pass_func pass);

/*!
 * s^c  (c=const)  -->  (s*s*s*...)^1
 * s^-c (c=const)  -->  (s*s*s*...)^-1
 */
int csfg_expr_op_expand_constant_exponents(struct csfg_expr_pool** pool);

/*! s^(a+b)        -->  s^a * s^b */
int csfg_expr_op_expand_exponent_sums(struct csfg_expr_pool** pool);

/*! (a*b)^c        -->  a^c * b^c */
int csfg_expr_op_expand_exponent_products(struct csfg_expr_pool** pool);

/*!
 * s*(a+b)        -->  s*a + s*b
 * (a+b)*s        -->  a*s + b*s
 * */
int csfg_expr_op_distribute_products(struct csfg_expr_pool** pool);

/*!
 * -(a*b)         --> -a*b
 * -s^a           --> (-1)*s^a
 */
int csfg_expr_op_lower_negates(struct csfg_expr_pool** pool);

/*!
 * a + s^-c (c=const)    -->  s^-c(a*s^c + 1)
 * a + b*s^-c (c=const)  -->  s^-c(a*s^c + b)
 */
int csfg_expr_op_factor_common_denominator(struct csfg_expr_pool** pool);

/*!
 * s*s                      --> s^2
 * s*s^c (c=const)          --> s^(c+1)
 * s^c1*s^c2 (c1,c2=const)  --> s^(c1+c2)
 */
int csfg_expr_op_simplify_products(struct csfg_expr_pool** pool);

/*!
 * s+s                      --> 2*s
 * s+c*s (c=const)          --> (c+1)*s
 * s*c1+s*c2 (c1,c2=const)  --> (c1+c2)*s
 */
int csfg_expr_op_simplify_sums(struct csfg_expr_pool** pool);

/*!
 * Moves all common reciprocs from numerator to denominator or vice-versa, such
 * that no more reciprocs exist. Combined with other operations, this will
 * normalize the fraction.
 */
int csfg_expr_op_rebalance_fraction(
    struct csfg_expr_pool** num_pool,
    int                     num_root,
    struct csfg_expr_pool** den_pool,
    int                     den_root);
