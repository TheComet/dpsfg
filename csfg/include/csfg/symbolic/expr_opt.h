#pragma once

struct csfg_expr_pool;

/*!
 * (const subtree) -->  c
 * -(c) (c=const)  -->  -c
 * 2+a+3           -->  5+a
 * 2*a*3           -->  6*a
 */
int csfg_expr_opt_fold_constants(struct csfg_expr_pool** pool, int* root);

/*!
 * -(-c)           -->  c
 * 0+a             -->  a
 * 1*a             -->  a
 * -1*a            -->  -a
 * a^1             -->  a
 * a^0             -->  1
 * a*a^-1          -->  1
 * a^-1^-1         -->  a
 */
int csfg_expr_opt_remove_useless_ops(struct csfg_expr_pool** pool, int* root);
