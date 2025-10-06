#pragma once

#include <stdarg.h>

struct csfg_expr_pool;
typedef int (*csfg_expr_pass_func)(struct csfg_expr_pool**);

/*!
 * Use this if you want to run multiple operations and combine their results.
 *
 * Will call each pass in sequence repeatedly until all return "0".
 * If any ops return 1, then this function will return 1.
 * If all ops return 0 initially, then this function will return 0.
 * If any ops return -1 at any point, this function returns -1 immediately.
 *
 * @param[in] ... List of @see csfg_expr_op_func function pointers. Must be
 * terminated by NULL.
 */
int csfg_expr_op_run(struct csfg_expr_pool** pool, ...);
int csfg_expr_op_runv(struct csfg_expr_pool** pool, va_list ap);

/*! This is used interally by all of the proceeding op functions as a
 * convenience. You shouldn't need to use this function externally ever */
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
 * s+s*c (c=const)          --> (c+1)*s
 * s*c1+s*c2 (c1,c2=const)  --> (c1+c2)*s
 * c1*s+s*c2 (c1,c2=const)  --> (c1+c2)*s
 * s*c1+c2*s (c1,c2=const)  --> (c1+c2)*s
 * c1*s+c2*s (c1,c2=const)  --> (c1+c2)*s
 */
int csfg_expr_op_simplify_sums(struct csfg_expr_pool** pool);
