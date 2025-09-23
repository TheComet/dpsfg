#pragma once

struct csfg_expr_pool;

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
