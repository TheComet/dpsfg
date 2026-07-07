#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/rulebook.h"
#include "csfg/symbolic/rules.h"
#include "csfg/symbolic/tf_expr.h"
#include "csfg/symbolic/var_table.h"
#include <math.h>

/* -------------------------------------------------------------------------- */
static int csfg_expr_to_rational_recurse(
    struct csfg_tf_expr* tf,
    struct csfg_expr_pool** pool,
    int expr,
    const char* variable)
{
    int left, right, k;
    struct strview var;
    struct csfg_coeff_expr* coeff;
    enum csfg_expr_type type;
    double value;

    struct csfg_tf_expr r1, r2;
    csfg_tf_expr_init(&r1);
    csfg_tf_expr_init(&r2);

    CSFG_DEBUG_ASSERT(vec_count(tf->num) == 0);
    CSFG_DEBUG_ASSERT(vec_count(tf->den) == 0);

    type  = (*pool)->nodes[expr].type;
    left  = (*pool)->nodes[expr].child[0];
    right = (*pool)->nodes[expr].child[1];

    switch (type)
    {
        case CSFG_EXPR_GC : assert(0); break;
        case CSFG_EXPR_LIT: {
            double value = (*pool)->nodes[expr].value.lit;
            if (csfg_poly_expr_push(&tf->num, csfg_coeff_expr(value, -1)) != 0)
                return -1;
            if (csfg_poly_expr_push(&tf->den, csfg_coeff_expr(1.0, -1)) != 0)
                return -1;
            break;
        }

        case CSFG_EXPR_IMAG:
        case CSFG_EXPR_INF : {
            if (csfg_poly_expr_push(&tf->num, csfg_coeff_expr(1.0, expr)) != 0)
                return -1;
            if (csfg_poly_expr_push(&tf->den, csfg_coeff_expr(1.0, -1)) != 0)
                return -1;
            break;
        }

        case CSFG_EXPR_VAR: {
            int var_idx = (*pool)->nodes[expr].value.var_idx;
            var         = strlist_view((*pool)->var_names, var_idx);
            if (strview_eq_cstr(var, variable))
            {
                /* 0 + 1*s */
                if (csfg_poly_expr_push(&tf->num, csfg_coeff_expr(0.0, -1)) !=
                    0)
                    return -1;
                if (csfg_poly_expr_push(&tf->num, csfg_coeff_expr(1.0, -1)) !=
                    0)
                    return -1;
            }
            else
            {
                if (csfg_poly_expr_push(&tf->num, csfg_coeff_expr(1.0, expr)) !=
                    0)
                    return -1;
            }
            if (csfg_poly_expr_push(&tf->den, csfg_coeff_expr(1.0, -1)) != 0)
                return -1;
            break;
        }

        case CSFG_EXPR_NEG: {
            if (csfg_expr_to_rational(tf, pool, left, variable) != 0)
                return -1;
            vec_for_each (tf->num, coeff)
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
            rc = csfg_expr_to_rational(&r1, pool, left, variable);
            if (rc != 0)
                return -1;

            /* rhs */
            rc = csfg_expr_to_rational(&r2, pool, right, variable);
            if (rc != 0)
                return -1;

            /* N1*D2 */
            rc = csfg_poly_expr_mul(pool, &tf->num, r1.num, r2.den);
            if (rc != 0)
                return -1;

            /* N2*D1 */
            rc = csfg_poly_expr_mul(pool, &r1.num, r2.num, r1.den);
            if (rc != 0)
                return -1;

            /* N1*D2 + N2*D1 */
            csfg_poly_expr_swap(&tf->num, &r2.num);
            rc = csfg_poly_expr_add(pool, &tf->num, r1.num, r2.num);
            if (rc != 0)
                return -1;

            /* D1*D2 */
            rc = csfg_poly_expr_mul(pool, &tf->den, r1.den, r2.den);
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
            rc = csfg_expr_to_rational(&r1, pool, left, variable);
            if (rc != 0)
                return -1;

            /* rhs */
            rc = csfg_expr_to_rational(&r2, pool, right, variable);
            if (rc != 0)
                return -1;

            /* N1*N2 */
            rc = csfg_poly_expr_mul(pool, &tf->num, r1.num, r2.num);
            if (rc != 0)
                return -1;

            /* D1*D2 */
            rc = csfg_poly_expr_mul(pool, &tf->den, r1.den, r2.den);
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
            if ((*pool)->nodes[right].type != CSFG_EXPR_LIT)
                return -1;

            value = (*pool)->nodes[right].value.lit;
            k     = (int)round(value);
            if (fabs(value - (double)k) >= 0.0000001)
                return -1;

            if (k == 0) /* a^0 = 1 */
            {
                if (csfg_poly_expr_push(&tf->num, csfg_coeff_expr(1.0, -1)) !=
                    0)
                    return -1;
                if (csfg_poly_expr_push(&tf->den, csfg_coeff_expr(1.0, -1)) !=
                    0)
                    return -1;
                break;
            }

            if (k > 0)
            {
                rc = csfg_expr_to_rational(&r1, pool, left, variable);
                if (rc != 0)
                    return -1;

                if (csfg_poly_expr_copy(&tf->num, r1.num) != 0)
                    return -1;
                if (csfg_poly_expr_copy(&tf->den, r1.den) != 0)
                    return -1;
                while (--k > 0)
                {
                    rc = csfg_poly_expr_mul(pool, &r2.num, tf->num, r1.num);
                    if (rc != 0)
                        return -1;
                    rc = csfg_poly_expr_mul(pool, &r2.den, tf->den, r1.den);
                    if (rc != 0)
                        return -1;
                    csfg_poly_expr_swap(&tf->num, &r2.num);
                    csfg_poly_expr_swap(&tf->den, &r2.den);
                }
            }
            else if (k < 0)
            {
                rc = csfg_expr_to_rational(&r2, pool, left, variable);
                if (rc != 0)
                    return -1;

                csfg_poly_expr_swap(&r2.num, &r1.den);
                csfg_poly_expr_swap(&r2.den, &r1.num);
                if (csfg_poly_expr_copy(&tf->num, r1.num) != 0)
                    return -1;
                if (csfg_poly_expr_copy(&tf->den, r1.den) != 0)
                    return -1;
                while (++k < 0)
                {
                    rc = csfg_poly_expr_mul(pool, &r2.num, tf->num, r1.num);
                    if (rc != 0)
                        return -1;
                    rc = csfg_poly_expr_mul(pool, &r2.den, tf->den, r1.den);
                    if (rc != 0)
                        return -1;
                    csfg_poly_expr_swap(&tf->num, &r2.num);
                    csfg_poly_expr_swap(&tf->den, &r2.den);
                }
            }
            break;
        }
    }

    csfg_tf_expr_deinit(&r1);
    csfg_tf_expr_deinit(&r2);

    return 0;
}
int csfg_expr_to_rational(
    struct csfg_tf_expr* tf,
    struct csfg_expr_pool** pool,
    int expr,
    const char* variable)
{
    csfg_tf_expr_clear(tf);
    return csfg_expr_to_rational_recurse(tf, pool, expr, variable);
}

/* -------------------------------------------------------------------------- */
int csfg_expr_to_rational_limit(
    struct csfg_tf_expr* tf,
    struct csfg_expr_pool** pool,
    int expr,
    const char* variable)
{
    int rc, num_idx, den_idx;
    const struct csfg_coeff_expr* c;

    rc = csfg_expr_to_rational(tf, pool, expr, variable);
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
        if (csfg_poly_expr_push(&tf->num, csfg_coeff_expr(0.0, -1)) != 0)
            return -1;
        if (csfg_poly_expr_push(&tf->den, csfg_coeff_expr(0.0, -1)) != 0)
            return -1;
    }
    else if (den_idx < 0) /* Denominator has factor 0 */
    {
        double factor = vec_get(tf->num, num_idx)->factor < 0.0 ? -1.0 : 1.0;
        int inf       = csfg_expr_inf(pool);
        if (inf < 0)
            return -1;
        csfg_tf_expr_clear(tf);
        if (csfg_poly_expr_push(&tf->num, csfg_coeff_expr(factor, inf)) != 0)
            return -1;
        if (csfg_poly_expr_push(&tf->den, csfg_coeff_expr(0.0, -1)) != 0)
            return -1;
    }
    else if (num_idx < 0) /* Numerator has factor 0 */
    {
        csfg_tf_expr_clear(tf);
        if (csfg_poly_expr_push(&tf->num, csfg_coeff_expr(0.0, -1)) != 0)
            return -1;
        if (csfg_poly_expr_push(&tf->den, csfg_coeff_expr(1.0, -1)) != 0)
            return -1;
    }
    else if (num_idx > den_idx) /* Numerator diverges to inf */
    {
        double factor = vec_get(tf->num, num_idx)->factor < 0.0 ? -1.0 : 1.0;
        int inf       = csfg_expr_inf(pool);
        if (inf < 0)
            return -1;
        csfg_tf_expr_clear(tf);
        if (csfg_poly_expr_push(&tf->num, csfg_coeff_expr(factor, inf)) != 0)
            return -1;
        if (csfg_poly_expr_push(&tf->den, csfg_coeff_expr(1.0, -1)) != 0)
            return -1;
    }
    else if (num_idx < den_idx) /* Denominator diverges to inf */
    {
        csfg_tf_expr_clear(tf);
        if (csfg_poly_expr_push(&tf->num, csfg_coeff_expr(0.0, -1)) != 0)
            return -1;
        if (csfg_poly_expr_push(&tf->den, csfg_coeff_expr(1.0, -1)) != 0)
            return -1;
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
    struct csfg_tf_expr* tf,
    struct csfg_expr_pool** pool,
    int expr,
    const struct csfg_var_table* vt)
{
    int slot, tmp_expr;
    struct str* key;
    struct csfg_var_table_entry* entry;

    tmp_expr = -1;
    hmap_for_each (vt->map, slot, key, entry)
    {
        const char* var_name = str_cstr(key);
        if (entry->pool->nodes[entry->expr].type != CSFG_EXPR_INF)
            continue;

        if (vec_count(tf->num) > 0)
        {
            tmp_expr = csfg_rational_to_expr(tf, pool, var_name);
            if (tmp_expr < 0)
                return -1;
            csfg_tf_expr_clear(tf);
            if (csfg_expr_to_rational_limit(tf, pool, tmp_expr, var_name) != 0)
                return -1;
        }
        else
        {
            if (csfg_expr_to_rational_limit(tf, pool, expr, var_name) != 0)
                return -1;
        }
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
int csfg_poly_expr_to_expr(
    const struct csfg_poly_expr* poly,
    struct csfg_expr_pool** pool,
    const char* variable)
{
    const struct csfg_coeff_expr* c;
    int n, coeff_expr, expr = -1;
    vec_enumerate (poly, n, c)
    {
        if (c->factor == 0.0)
            continue;

        /* Convert term into an expression, including factor */
        if (c->expr < 0)
            coeff_expr = csfg_expr_lit(pool, c->factor);
        else
            coeff_expr = csfg_expr_mul(
                pool,
                csfg_expr_dup_recurse(pool, c->expr),
                csfg_expr_lit(pool, c->factor));

        /* Multiply by polynomial variable */
        if (n == 1)
            coeff_expr = csfg_expr_mul(
                pool, coeff_expr, csfg_expr_var(pool, cstr_view(variable)));
        if (n > 1)
            coeff_expr = csfg_expr_mul(
                pool,
                coeff_expr,
                csfg_expr_pow(
                    pool,
                    csfg_expr_var(pool, cstr_view(variable)),
                    csfg_expr_lit(pool, n)));

        /* Add to previous term, if any */
        if (expr < 0)
            expr = coeff_expr;
        else
            expr = csfg_expr_add(pool, expr, coeff_expr);

        if (expr < 0)
            return -1;
    }

    return expr;
}

/* -------------------------------------------------------------------------- */
int csfg_rational_to_expr(
    const struct csfg_tf_expr* tf,
    struct csfg_expr_pool** pool,
    const char* variable)
{
    return csfg_expr_div(
        pool,
        csfg_poly_expr_to_expr(tf->num, pool, variable),
        csfg_poly_expr_to_expr(tf->den, pool, variable));
}

VEC_DEFINE(csfg_expr_vec, int, 8)

/* -------------------------------------------------------------------------- */
static int run(struct csfg_expr_pool** num, struct csfg_expr_pool** den, ...)
{
    va_list ap;
    int result, modified = 0;

    va_start(ap, den);
    result = csfg_rules_runv(num, ap);
    va_end(ap);
    if (result == -1)
        return -1;
    if (result == 1)
        modified = 1;

    va_start(ap, den);
    result = csfg_rules_runv(den, ap);
    va_end(ap);
    if (result == -1)
        return -1;
    if (result == 1)
        modified = 1;

    return modified;
}

/* -------------------------------------------------------------------------- */
/* TODO: Delete this function (is unused, only for reference for previous
 * implementation) */
int csfg_expr_to_standard_tf(
    struct csfg_expr_pool** num_pool,
    int* num_root,
    struct csfg_expr_pool** den_pool,
    int* den_root)
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
                csfg_rule_factor_common_denominator,
                NULL) == -1)
        {
            return -1;
        }
        *num_root = csfg_expr_gc(*num_pool, *num_root);
        *den_root = csfg_expr_gc(*den_pool, *den_root);

        if (run(num_pool,
                den_pool,
                csfg_rule_expand_constant_exponents,
                csfg_rule_expand_exponent_sums,
                csfg_rule_expand_exponent_products,
                csfg_rule_distribute_products,
                NULL) == -1)
        {
            return -1;
        }

        if (run(num_pool,
                den_pool,
                csfg_rule_fold_constants,
                csfg_rule_remove_useless_ops,
                NULL) == -1)
        {
            return -1;
        }
        *num_root = csfg_expr_gc(*num_pool, *num_root);
        *den_root = csfg_expr_gc(*den_pool, *den_root);

        /* rebalance fraction requires this pass */
        if (run(num_pool, den_pool, csfg_rule_lower_negates, NULL) == -1)
            return -1;
        *num_root = csfg_expr_gc(*num_pool, *num_root);
        *den_root = csfg_expr_gc(*den_pool, *den_root);

        switch (csfg_expr_rebalance_fraction(
            num_pool, *num_root, den_pool, *den_root))
        {
            case -1: return -1;
            case 0 : done = 1; break;
            case 1 : break;
        }
        *num_root = csfg_expr_gc(*num_pool, *num_root);
        *den_root = csfg_expr_gc(*den_pool, *den_root);

        if (done)
            break;
    }

    if (run(num_pool,
            den_pool,
            csfg_rule_expand_constant_exponents,
            csfg_rule_expand_exponent_sums,
            csfg_rule_expand_exponent_products,
            csfg_rule_distribute_products,
            NULL) == -1)
        return -1;
    if (run(num_pool,
            den_pool,
            csfg_rule_fold_constants,
            csfg_rule_remove_useless_ops,
            csfg_rule_simplify_products,
            csfg_rule_simplify_sums,
            NULL) == -1)
        return -1;
    *num_root = csfg_expr_gc(*num_pool, *num_root);
    *den_root = csfg_expr_gc(*den_pool, *den_root);

    return 0;
}
