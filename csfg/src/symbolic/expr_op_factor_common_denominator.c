#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"

/* ------------------------------------------------------------------------- */
static int find_power_with_negative_constant_exponent(
    const struct csfg_expr_pool* pool, int n)
{
    int left = pool->nodes[n].child[0];
    int right = pool->nodes[n].child[1];
    if (pool->nodes[n].type == CSFG_EXPR_OP_MUL)
    {
        n = find_power_with_negative_constant_exponent(pool, left);
        if (n != -1)
            return n;
        n = find_power_with_negative_constant_exponent(pool, right);
        if (n != -1)
            return n;
    }
    else if (pool->nodes[n].type == CSFG_EXPR_OP_POW)
    {
        int exp = pool->nodes[n].child[1];
        if (pool->nodes[exp].type != CSFG_EXPR_LIT)
            return -1;
        if (pool->nodes[exp].value.lit >= 0.0)
            return -1;
        return n;
    }

    return -1;
}

/* ------------------------------------------------------------------------- */
static int factor_common_denominator(struct csfg_expr_pool** pool)
{
    int n, modified = 0;
    for (n = 0; n != (*pool)->count; ++n)
    {
        int    other_summand, pow, product, exp;
        double neg_exp;
        int    left = (*pool)->nodes[n].child[0];
        int    right = (*pool)->nodes[n].child[1];

        if ((*pool)->nodes[n].type != CSFG_EXPR_OP_ADD)
            continue;

        pow = find_power_with_negative_constant_exponent(*pool, left);
        other_summand = right;
        if (pow == -1)
        {
            pow = find_power_with_negative_constant_exponent(*pool, right);
            other_summand = left;
        }
        if (pow == -1)
            continue;

        exp = (*pool)->nodes[pow].child[1];
        neg_exp = -(*pool)->nodes[exp].value.lit;

        product = csfg_expr_find_parent(*pool, pow);
        if ((*pool)->nodes[product].type == CSFG_EXPR_OP_MUL)
        {
            int mul = (*pool)->nodes[product].child[0] == pow
                          ? (*pool)->nodes[product].child[1]
                          : (*pool)->nodes[product].child[0];
            /* mul and pow dangle after this */
            (*pool)->nodes[product] = (*pool)->nodes[mul];
            (*pool)->nodes[mul] = (*pool)->nodes[other_summand];
            csfg_expr_set_mul(
                pool,
                other_summand,
                mul,
                csfg_expr_pow(
                    pool,
                    csfg_expr_dup_recurse(pool, (*pool)->nodes[pow].child[0]),
                    csfg_expr_lit(pool, neg_exp)));
            csfg_expr_set_mul(
                pool,
                n,
                pow,
                csfg_expr_add(
                    pool,
                    (*pool)->nodes[n].child[0],
                    (*pool)->nodes[n].child[1]));
            modified = 1;
        }
        else
        {
            if (csfg_expr_set_mul(
                    pool,
                    n,
                    pow,
                    csfg_expr_add(
                        pool,
                        csfg_expr_mul(
                            pool,
                            other_summand,
                            csfg_expr_pow(
                                pool,
                                csfg_expr_dup_recurse(
                                    pool, (*pool)->nodes[pow].child[0]),
                                csfg_expr_lit(pool, neg_exp))),
                        csfg_expr_lit(pool, 1.0))) == -1)
            {
                return -1;
            }
            modified = 1;
        }
    }

    return modified;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_op_factor_common_denominator(struct csfg_expr_pool** pool)
{
    return csfg_expr_op_run_pass(pool, factor_common_denominator);
}
