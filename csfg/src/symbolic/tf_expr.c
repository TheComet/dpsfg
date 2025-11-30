#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"
#include "csfg/symbolic/expr_opt.h"
#include "csfg/symbolic/tf_expr.h"
#include "csfg/symbolic/var_table.h"
#include <math.h>

/* -------------------------------------------------------------------------- */
int csfg_expr_to_rational(
    const struct csfg_expr_pool* in_pool,
    int                          in_expr,
    struct strview               main_var,
    struct csfg_expr_pool**      tf_pool,
    struct csfg_tf_expr*         r)
{
    int                     left, right, k;
    struct strview          var;
    struct csfg_coeff_expr* coeff;
    double                  value;

    struct csfg_tf_expr r1, r2;
    csfg_tf_expr_init(&r1);
    csfg_tf_expr_init(&r2);

    CSFG_DEBUG_ASSERT(vec_count(r->num) == 0);
    CSFG_DEBUG_ASSERT(vec_count(r->den) == 0);

    left = in_pool->nodes[in_expr].child[0];
    right = in_pool->nodes[in_expr].child[1];

    switch (in_pool->nodes[in_expr].type)
    {
        case CSFG_EXPR_GC: assert(0); break;
        case CSFG_EXPR_LIT: {
            double value = in_pool->nodes[in_expr].value.lit;
            if (csfg_poly_expr_push(&r->num, csfg_coeff_expr(value, -1)) != 0)
                return -1;
            if (csfg_poly_expr_push(&r->den, csfg_coeff_expr(1.0, -1)) != 0)
                return -1;
            break;
        }

        case CSFG_EXPR_INF: {
            int n = csfg_expr_dup_single_from(tf_pool, in_pool, in_expr);
            if (n < 0)
                return -1;
            if (csfg_poly_expr_push(&r->num, csfg_coeff_expr(1.0, n)) != 0)
                return -1;
            if (csfg_poly_expr_push(&r->den, csfg_coeff_expr(1.0, -1)) != 0)
                return -1;
            break;
        }

        case CSFG_EXPR_VAR: {
            int var_idx = in_pool->nodes[in_expr].value.var_idx;
            var = strlist_view(in_pool->var_names, var_idx);
            if (strview_eq(var, main_var))
            {
                /* 0 + 1*s */
                if (csfg_poly_expr_push(&r->num, csfg_coeff_expr(0.0, -1)) != 0)
                    return -1;
                if (csfg_poly_expr_push(&r->num, csfg_coeff_expr(1.0, -1)) != 0)
                    return -1;
            }
            else
            {
                int n;
                n = csfg_expr_dup_single_from(tf_pool, in_pool, in_expr);
                if (n < 0)
                    return -1;
                if (csfg_poly_expr_push(&r->num, csfg_coeff_expr(1.0, n)) != 0)
                    return -1;
            }
            csfg_poly_expr_push(&r->den, csfg_coeff_expr(1.0, -1));
            break;
        }

        case CSFG_EXPR_NEG: {
            int result =
                csfg_expr_to_rational(in_pool, left, main_var, tf_pool, r);
            if (result != 0)
                return -1;
            vec_for_each (r->num, coeff)
                coeff->factor *= -1;
            break;
        }

        case CSFG_EXPR_ADD: {
            int rc;
            /*
             * N1   N2   N1*D2 + N2*D1
             * -- + -- = -------------
             * D1   D2       D1*D2
             */

            /* lhs */
            rc = csfg_expr_to_rational(in_pool, left, main_var, tf_pool, &r1);
            if (rc != 0)
                return -1;

            /* rhs */
            rc = csfg_expr_to_rational(in_pool, right, main_var, tf_pool, &r2);
            if (rc != 0)
                return -1;

            /* N1*D2 */
            rc = csfg_poly_expr_mul(tf_pool, &r->num, r1.num, r2.den);
            if (rc != 0)
                return -1;

            /* N2*D1 */
            csfg_poly_expr_clear(r1.num);
            rc = csfg_poly_expr_mul(tf_pool, &r1.num, r2.num, r1.den);
            if (rc != 0)
                return -1;

            /* N1*D2 + N2*D1 */
            csfg_poly_expr_swap(&r->num, &r2.num);
            csfg_poly_expr_clear(r->num);
            rc = csfg_poly_expr_add(tf_pool, &r->num, r1.num, r2.num);
            if (rc != 0)
                return -1;

            /* D1*D2 */
            rc = csfg_poly_expr_mul(tf_pool, &r->den, r1.den, r2.den);
            if (rc != 0)
                return -1;
            break;
        }

        case CSFG_EXPR_MUL: {
            int rc;
            /*
             * N1   N2   N1*N2
             * -- * -- = -----
             * D1   D2   D1*D2
             */

            /* lhs */
            rc = csfg_expr_to_rational(in_pool, left, main_var, tf_pool, &r1);
            if (rc != 0)
                return -1;

            /* rhs */
            rc = csfg_expr_to_rational(in_pool, right, main_var, tf_pool, &r2);
            if (rc != 0)
                return -1;

            /* N1*N2 */
            rc = csfg_poly_expr_mul(tf_pool, &r->num, r1.num, r2.num);
            if (rc != 0)
                return -1;

            /* D1*D2 */
            rc = csfg_poly_expr_mul(tf_pool, &r->den, r1.den, r2.den);
            if (rc != 0)
                return -1;
            break;
        }

        case CSFG_EXPR_POW: {
            int rc;
            /*
             *     k
             * /N1\   N1^k   D1^-k
             * |--| = ---- = -----
             * \D1/   D1^k   N1^-k
             */
            if (in_pool->nodes[right].type != CSFG_EXPR_LIT)
                return -1;

            value = in_pool->nodes[right].value.lit;
            k = (int)round(value);
            if (fabs(value - (double)k) >= 0.0000001)
                return -1;

            if (k == 0) /* a^0 = 1 */
            {
                if (csfg_poly_expr_push(&r->num, csfg_coeff_expr(1.0, -1)) != 0)
                    return -1;
                if (csfg_poly_expr_push(&r->den, csfg_coeff_expr(1.0, -1)) != 0)
                    return -1;
                break;
            }

            if (k > 0)
            {
                rc = csfg_expr_to_rational(
                    in_pool, left, main_var, tf_pool, &r1);
                if (rc != 0)
                    return -1;

                if (csfg_poly_expr_copy(&r->num, r1.num) != 0)
                    return -1;
                if (csfg_poly_expr_copy(&r->den, r1.den) != 0)
                    return -1;
                while (--k > 0)
                {
                    csfg_tf_expr_clear(&r2);
                    rc = csfg_poly_expr_mul(tf_pool, &r2.num, r->num, r1.num);
                    if (rc != 0)
                        return -1;
                    rc = csfg_poly_expr_mul(tf_pool, &r2.den, r->den, r1.den);
                    if (rc != 0)
                        return -1;
                    csfg_poly_expr_swap(&r->num, &r2.num);
                    csfg_poly_expr_swap(&r->den, &r2.den);
                }
            }
            else if (k < 0)
            {
                rc = csfg_expr_to_rational(
                    in_pool, left, main_var, tf_pool, &r2);
                if (rc != 0)
                    return -1;

                csfg_poly_expr_swap(&r2.num, &r1.den);
                csfg_poly_expr_swap(&r2.den, &r1.num);
                if (csfg_poly_expr_copy(&r->num, r1.num) != 0)
                    return -1;
                if (csfg_poly_expr_copy(&r->den, r1.den) != 0)
                    return -1;
                while (++k < 0)
                {
                    csfg_tf_expr_clear(&r2);
                    rc = csfg_poly_expr_mul(tf_pool, &r2.num, r->num, r1.num);
                    if (rc != 0)
                        return -1;
                    rc = csfg_poly_expr_mul(tf_pool, &r2.den, r->den, r1.den);
                    if (rc != 0)
                        return -1;
                    csfg_poly_expr_swap(&r->num, &r2.num);
                    csfg_poly_expr_swap(&r->den, &r2.den);
                }
            }
            break;
        }
    }

    csfg_tf_expr_deinit(&r1);
    csfg_tf_expr_deinit(&r2);

    return 0;
}

/* -------------------------------------------------------------------------- */
int csfg_expr_to_rational_limit(
    const struct csfg_expr_pool* in_pool,
    int                          in_expr,
    struct strview               variable,
    struct csfg_expr_pool**      tf_pool,
    struct csfg_tf_expr*         tf)
{
    int                           rc, num_idx, den_idx;
    const struct csfg_coeff_expr* c;

    CSFG_DEBUG_ASSERT(vec_count(tf->num) == 0);
    CSFG_DEBUG_ASSERT(vec_count(tf->den) == 0);

    rc = csfg_expr_to_rational(in_pool, in_expr, variable, tf_pool, tf);
    if (rc != 0)
        return -1;

    /* Find the highest, non-zero coefficients */
    vec_enumerate_r (tf->num, num_idx, c)
        if (c->factor != 0.0)
            break;
    vec_enumerate_r (tf->den, den_idx, c)
        if (c->factor != 0.0)
            break;

    if (den_idx < 0 && num_idx < 0) /* 0/0 */
    {
        csfg_tf_expr_clear(tf);
        csfg_poly_expr_push(&tf->num, csfg_coeff_expr(0.0, -1));
        csfg_poly_expr_push(&tf->den, csfg_coeff_expr(0.0, -1));
    }
    else if (den_idx < 0) /* Denominator has factor 0 */
    {
        double factor = vec_get(tf->num, num_idx)->factor < 0.0 ? -1.0 : 1.0;
        int    inf = csfg_expr_inf(tf_pool);
        if (inf < 0)
            return -1;
        csfg_tf_expr_clear(tf);
        csfg_poly_expr_push(&tf->num, csfg_coeff_expr(factor, inf));
        csfg_poly_expr_push(&tf->den, csfg_coeff_expr(0.0, -1));
    }
    else if (num_idx < 0) /* Numerator has factor 0 */
    {
        csfg_tf_expr_clear(tf);
        csfg_poly_expr_push(&tf->num, csfg_coeff_expr(0.0, -1));
        csfg_poly_expr_push(&tf->den, csfg_coeff_expr(1.0, -1));
    }
    else if (num_idx > den_idx) /* Numerator diverges to inf */
    {
        double factor = vec_get(tf->num, num_idx)->factor < 0.0 ? -1.0 : 1.0;
        int    inf = csfg_expr_inf(tf_pool);
        if (inf < 0)
            return -1;
        csfg_tf_expr_clear(tf);
        csfg_poly_expr_push(&tf->num, csfg_coeff_expr(factor, inf));
        csfg_poly_expr_push(&tf->den, csfg_coeff_expr(1.0, -1));
    }
    else if (num_idx < den_idx) /* Denominator diverges to inf */
    {
        csfg_tf_expr_clear(tf);
        csfg_poly_expr_push(&tf->num, csfg_coeff_expr(0.0, -1));
        csfg_poly_expr_push(&tf->den, csfg_coeff_expr(1.0, -1));
    }
    else /* Convergent */
    {
        csfg_poly_expr_swap_values(tf->num, 0, num_idx);
        csfg_poly_expr_swap_values(tf->den, 0, den_idx);
        tf->num->count = 1;
        tf->den->count = 1;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
int csfg_expr_to_rational_limits(
    const struct csfg_expr_pool* in_pool,
    int                          in_expr,
    const struct csfg_var_table* vt,
    struct csfg_expr_pool**      tf_pool,
    struct csfg_tf_expr*         tf)
{
    int                          slot, tmp_expr;
    struct str*                  key;
    struct csfg_var_table_entry* entry;
    struct csfg_expr_pool*       tmp_pool;

    CSFG_DEBUG_ASSERT(vec_count(tf->num) == 0);
    CSFG_DEBUG_ASSERT(vec_count(tf->den) == 0);

    csfg_expr_pool_init(&tmp_pool);

    tmp_expr = -1;
    hmap_for_each (vt->map, slot, key, entry)
    {
        struct strview var_name = str_view(key);
        if (entry->pool->nodes[entry->expr].type != CSFG_EXPR_INF)
            continue;

        if (vec_count(tf->num) > 0)
        {
            tmp_expr = csfg_rational_to_expr(tf, *tf_pool, &tmp_pool);
            if (tmp_expr < 0)
                goto fail;
            csfg_tf_expr_clear(tf);
            if (csfg_expr_to_rational_limit(
                    tmp_pool, tmp_expr, var_name, tf_pool, tf) != 0)
                goto fail;
        }
        else
        {
            if (csfg_expr_to_rational_limit(
                    in_pool, in_expr, var_name, tf_pool, tf) != 0)
                goto fail;
        }
    }

    csfg_expr_pool_deinit(tmp_pool);
    return 0;

fail:
    csfg_expr_pool_deinit(tmp_pool);
    return -1;
}

/* -------------------------------------------------------------------------- */
int csfg_poly_expr_to_expr(
    const struct csfg_poly_expr* poly,
    const struct csfg_expr_pool* coeff_pool,
    struct csfg_expr_pool**      expr_pool)
{
    const struct csfg_coeff_expr* c;
    int                           expr = -1;
    vec_for_each (poly, c)
    {
        int coeff_expr;
        if (c->factor == 0.0)
            continue;

        if (c->expr < 0)
            coeff_expr = csfg_expr_lit(expr_pool, c->factor);
        else
            coeff_expr = csfg_expr_mul(
                expr_pool,
                csfg_expr_dup_recurse_from(expr_pool, coeff_pool, c->expr),
                csfg_expr_lit(expr_pool, c->factor));

        if (expr == -1)
            expr = coeff_expr;
        else
            expr = csfg_expr_add(expr_pool, expr, coeff_expr);

        if (expr < 0)
            return -1;
    }

    return expr;
}

/* -------------------------------------------------------------------------- */
int csfg_rational_to_expr(
    const struct csfg_tf_expr*   tf,
    const struct csfg_expr_pool* tf_pool,
    struct csfg_expr_pool**      expr_pool)
{
    return csfg_expr_div(
        expr_pool,
        csfg_poly_expr_to_expr(tf->num, tf_pool, expr_pool),
        csfg_poly_expr_to_expr(tf->den, tf_pool, expr_pool));
}

VEC_DEFINE(csfg_expr_vec, int, 8)

/* -------------------------------------------------------------------------- */
static int run(struct csfg_expr_pool** num, struct csfg_expr_pool** den, ...)
{
    va_list ap;
    int     result, modified = 0;

    va_start(ap, den);
    result = csfg_expr_op_runv(num, ap);
    va_end(ap);
    if (result == -1)
        return -1;
    if (result == 1)
        modified = 1;

    va_start(ap, den);
    result = csfg_expr_op_runv(den, ap);
    va_end(ap);
    if (result == -1)
        return -1;
    if (result == 1)
        modified = 1;

    return modified;
}

/* -------------------------------------------------------------------------- */
int csfg_expr_to_standard_tf(
    struct csfg_expr_pool** num_pool,
    int*                    num_root,
    struct csfg_expr_pool** den_pool,
    int*                    den_root)
{
    /* Init the denominator by dividing by 1.0 */
    csfg_expr_pool_clear(*den_pool);
    *den_root = csfg_expr_lit(den_pool, 1.0);
    if (*den_root == -1)
        return -1;

    while (1)
    {
        int done = 0;

        if (run(num_pool,
                den_pool,
                csfg_expr_op_factor_common_denominator,
                NULL) == -1)
        {
            return -1;
        }
        *num_root = csfg_expr_gc(*num_pool, *num_root);
        *den_root = csfg_expr_gc(*den_pool, *den_root);

        if (run(num_pool,
                den_pool,
                csfg_expr_op_expand_constant_exponents,
                csfg_expr_op_expand_exponent_sums,
                csfg_expr_op_expand_exponent_products,
                csfg_expr_op_distribute_products,
                NULL) == -1)
        {
            return -1;
        }

        if (run(num_pool,
                den_pool,
                csfg_expr_opt_fold_constants,
                csfg_expr_opt_remove_useless_ops,
                NULL) == -1)
        {
            return -1;
        }
        *num_root = csfg_expr_gc(*num_pool, *num_root);
        *den_root = csfg_expr_gc(*den_pool, *den_root);

        /* rebalance fraction requires this pass */
        if (run(num_pool, den_pool, csfg_expr_op_lower_negates, NULL) == -1)
            return -1;
        *num_root = csfg_expr_gc(*num_pool, *num_root);
        *den_root = csfg_expr_gc(*den_pool, *den_root);

        switch (csfg_expr_rebalance_fraction(
            num_pool, *num_root, den_pool, *den_root))
        {
            case -1: return -1;
            case 0: done = 1; break;
            case 1: break;
        }
        *num_root = csfg_expr_gc(*num_pool, *num_root);
        *den_root = csfg_expr_gc(*den_pool, *den_root);

        if (done)
            break;
    }

    if (run(num_pool,
            den_pool,
            csfg_expr_op_expand_constant_exponents,
            csfg_expr_op_expand_exponent_sums,
            csfg_expr_op_expand_exponent_products,
            csfg_expr_op_distribute_products,
            NULL) == -1)
        return -1;
    if (run(num_pool,
            den_pool,
            csfg_expr_opt_fold_constants,
            csfg_expr_opt_remove_useless_ops,
            csfg_expr_op_simplify_products,
            csfg_expr_op_simplify_sums,
            NULL) == -1)
        return -1;
    *num_root = csfg_expr_gc(*num_pool, *num_root);
    *den_root = csfg_expr_gc(*den_pool, *den_root);

    return 0;
}
