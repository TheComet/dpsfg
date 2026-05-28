#pragma once

#include <stdarg.h>

struct csfg_expr_pool;

typedef int (*csfg_rule_run_func)(struct csfg_expr_pool**);

/*!
 * \brief Will call each rule in sequence repeatedly until all return "0". The
 * result of each rule is combined into a final result and returned.
 * \param[in] ... List of \see csfg_rule_run_func function pointers. Must be
 * terminated by NULL.
 * \return If any rules return 1, then this function will return 1.
 * If all rules return 0 initially, then this function will return 0.
 * If any rules return -1 at any point, this function returns -1 immediately.
 */
int csfg_rules_run(struct csfg_expr_pool** pool, ...);
int csfg_rules_runv(struct csfg_expr_pool** pool, va_list ap);

/*! This is used interally by all of the expr_rules.h functions as a
 * convenience. You shouldn't need to use this function externally ever */
int csfg_rule_run(struct csfg_expr_pool** pool, csfg_rule_run_func pass);

/*!
 * (const subtree) -->  c
 * -(c) (c=const)  -->  -c
 * 2+a+3           -->  5+a
 * 2*a*3           -->  6*a
 */
int csfg_rule_fold_constants(struct csfg_expr_pool** pool);

/*!
 * -(-c)           -->  c
 * 0+a             -->  a
 * 1*a             -->  a
 * -1*a            --> -a
 * a^1             -->  a
 * a^0             -->  1
 * a*a^-1          -->  1
 * a^-1^-1         -->  a
 */
int csfg_rule_remove_useless_ops(struct csfg_expr_pool** pool);

/*!
 * s^c  (c=const)  -->  (s*s*s*...)^1
 * s^-c (c=const)  -->  (s*s*s*...)^-1
 */
int csfg_rule_expand_constant_exponents(struct csfg_expr_pool** pool);

/*! s^(a+b)        -->  s^a * s^b */
int csfg_rule_expand_exponent_sums(struct csfg_expr_pool** pool);

/*! (a*b)^c        -->  a^c * b^c */
int csfg_rule_expand_exponent_products(struct csfg_expr_pool** pool);

/*!
 * s*(a+b)        -->  s*a + s*b
 * (a+b)*s        -->  a*s + b*s
 * */
int csfg_rule_distribute_products(struct csfg_expr_pool** pool);

/*!
 * -(a*b)         --> -a*b
 * -s^a           --> (-1)*s^a
 */
int csfg_rule_lower_negates(struct csfg_expr_pool** pool);

/*!
 * a + s^-c (c=const)    -->  s^-c(a*s^c + 1)
 * a + b*s^-c (c=const)  -->  s^-c(a*s^c + b)
 */
int csfg_rule_factor_common_denominator(struct csfg_expr_pool** pool);

/*!
 * s*s                      --> s^2
 * s*s^c (c=const)          --> s^(c+1)
 * s^c1*s^c2 (c1,c2=const)  --> s^(c1+c2)
 */
int csfg_rule_simplify_products(struct csfg_expr_pool** pool);

/*!
 * s+s                      --> 2*s
 * s+c*s (c=const)          --> (c+1)*s
 * s+s*c (c=const)          --> (c+1)*s
 * s*c1+s*c2 (c1,c2=const)  --> (c1+c2)*s
 * c1*s+s*c2 (c1,c2=const)  --> (c1+c2)*s
 * s*c1+c2*s (c1,c2=const)  --> (c1+c2)*s
 * c1*s+c2*s (c1,c2=const)  --> (c1+c2)*s
 */
int csfg_rule_simplify_sums(struct csfg_expr_pool** pool);
