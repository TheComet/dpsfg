#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"
#include "csfg/symbolic/rational.h"
#include "csfg/symbolic/var_table.h"
#include <math.h>

/* ------------------------------------------------------------------------- */
int csfg_expr_to_rational(
    const struct csfg_expr_pool* in_pool,
    int                          in_expr,
    struct strview               main_var,
    struct csfg_expr_pool**      rational_pool,
    struct csfg_rational*        r)
{
    int                left, right, k;
    struct strview     var;
    struct csfg_coeff* coeff;
    double             value;

    struct csfg_rational r1, r2;
    csfg_rational_init(&r1);
    csfg_rational_init(&r2);

    CSFG_DEBUG_ASSERT(vec_count(r->num) == 0);
    CSFG_DEBUG_ASSERT(vec_count(r->den) == 0);

    left = in_pool->nodes[in_expr].child[0];
    right = in_pool->nodes[in_expr].child[1];

    switch (in_pool->nodes[in_expr].type)
    {
        case CSFG_EXPR_GC: assert(0); break;
        case CSFG_EXPR_LIT: {
            double value = in_pool->nodes[in_expr].value.lit;
            if (csfg_poly_push(&r->num, csfg_coeff(value, -1)) != 0)
                return -1;
            if (csfg_poly_push(&r->den, csfg_coeff(1.0, -1)) != 0)
                return -1;
            break;
        }

        case CSFG_EXPR_INF: {
            int n = csfg_expr_dup_single_from(rational_pool, in_pool, in_expr);
            if (n < 0)
                return -1;
            if (csfg_poly_push(&r->num, csfg_coeff(1.0, n)) != 0)
                return -1;
            if (csfg_poly_push(&r->den, csfg_coeff(1.0, -1)) != 0)
                return -1;
            break;
        }

        case CSFG_EXPR_VAR: {
            int var_idx = in_pool->nodes[in_expr].value.var_idx;
            var = strlist_view(in_pool->var_names, var_idx);
            if (strview_eq(var, main_var))
            {
                /* 0 + 1*s */
                if (csfg_poly_push(&r->num, csfg_coeff(0.0, -1)) != 0)
                    return -1;
                if (csfg_poly_push(&r->num, csfg_coeff(1.0, -1)) != 0)
                    return -1;
            }
            else
            {
                int n;
                n = csfg_expr_dup_single_from(rational_pool, in_pool, in_expr);
                if (n < 0)
                    return -1;
                if (csfg_poly_push(&r->num, csfg_coeff(1.0, n)) != 0)
                    return -1;
            }
            csfg_poly_push(&r->den, csfg_coeff(1.0, -1));
            break;
        }

        case CSFG_EXPR_NEG: {
            int result = csfg_expr_to_rational(
                in_pool, left, main_var, rational_pool, r);
            if (result != 0)
                return -1;
            vec_for_each (r->num, coeff)
                coeff->factor *= -1;
            break;
        }

        case CSFG_EXPR_OP_ADD: {
            int rc;
            /*
             * N1   N2   N1*D2 + N2*D1
             * -- + -- = -------------
             * D1   D2       D1*D2
             */

            /* lhs */
            rc = csfg_expr_to_rational(
                in_pool, left, main_var, rational_pool, &r1);
            if (rc != 0)
                return -1;

            /* rhs */
            rc = csfg_expr_to_rational(
                in_pool, right, main_var, rational_pool, &r2);
            if (rc != 0)
                return -1;

            /* N1*D2 */
            rc = csfg_poly_mul(rational_pool, &r->num, r1.num, r2.den);
            if (rc != 0)
                return -1;

            /* N2*D1 */
            csfg_poly_clear(r1.num);
            rc = csfg_poly_mul(rational_pool, &r1.num, r2.num, r1.den);
            if (rc != 0)
                return -1;

            /* N1*D2 + N2*D1 */
            csfg_poly_swap(&r->num, &r2.num);
            csfg_poly_clear(r->num);
            rc = csfg_poly_add(rational_pool, &r->num, r1.num, r2.num);
            if (rc != 0)
                return -1;

            /* D1*D2 */
            rc = csfg_poly_mul(rational_pool, &r->den, r1.den, r2.den);
            if (rc != 0)
                return -1;
            break;
        }

        case CSFG_EXPR_OP_MUL: {
            int rc;
            /*
             * N1   N2   N1*N2
             * -- * -- = -----
             * D1   D2   D1*D2
             */

            /* lhs */
            rc = csfg_expr_to_rational(
                in_pool, left, main_var, rational_pool, &r1);
            if (rc != 0)
                return -1;

            /* rhs */
            rc = csfg_expr_to_rational(
                in_pool, right, main_var, rational_pool, &r2);
            if (rc != 0)
                return -1;

            /* N1*N2 */
            rc = csfg_poly_mul(rational_pool, &r->num, r1.num, r2.num);
            if (rc != 0)
                return -1;

            /* D1*D2 */
            rc = csfg_poly_mul(rational_pool, &r->den, r1.den, r2.den);
            if (rc != 0)
                return -1;
            break;
        }

        case CSFG_EXPR_OP_POW: {
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
                if (csfg_poly_push(&r->num, csfg_coeff(1.0, -1)) != 0)
                    return -1;
                if (csfg_poly_push(&r->den, csfg_coeff(1.0, -1)) != 0)
                    return -1;
                break;
            }

            if (k > 0)
            {
                rc = csfg_expr_to_rational(
                    in_pool, left, main_var, rational_pool, &r1);
                if (rc != 0)
                    return -1;

                if (csfg_poly_copy(&r->num, r1.num) != 0)
                    return -1;
                if (csfg_poly_copy(&r->den, r1.den) != 0)
                    return -1;
                while (--k > 0)
                {
                    csfg_rational_clear(&r2);
                    rc = csfg_poly_mul(rational_pool, &r2.num, r->num, r1.num);
                    if (rc != 0)
                        return -1;
                    rc = csfg_poly_mul(rational_pool, &r2.den, r->den, r1.den);
                    if (rc != 0)
                        return -1;
                    csfg_poly_swap(&r->num, &r2.num);
                    csfg_poly_swap(&r->den, &r2.den);
                }
            }
            else if (k < 0)
            {
                rc = csfg_expr_to_rational(
                    in_pool, left, main_var, rational_pool, &r2);
                if (rc != 0)
                    return -1;

                csfg_poly_swap(&r2.num, &r1.den);
                csfg_poly_swap(&r2.den, &r1.num);
                if (csfg_poly_copy(&r->num, r1.num) != 0)
                    return -1;
                if (csfg_poly_copy(&r->den, r1.den) != 0)
                    return -1;
                while (++k < 0)
                {
                    csfg_rational_clear(&r2);
                    rc = csfg_poly_mul(rational_pool, &r2.num, r->num, r1.num);
                    if (rc != 0)
                        return -1;
                    rc = csfg_poly_mul(rational_pool, &r2.den, r->den, r1.den);
                    if (rc != 0)
                        return -1;
                    csfg_poly_swap(&r->num, &r2.num);
                    csfg_poly_swap(&r->den, &r2.den);
                }
            }
            break;
        }
    }

    csfg_rational_deinit(&r1);
    csfg_rational_deinit(&r2);

    return 0;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_to_rational_limit(
    const struct csfg_expr_pool* in_pool,
    int                          in_expr,
    struct strview               variable,
    struct csfg_expr_pool**      rational_pool,
    struct csfg_rational*        rational)
{
    int                      rc, num_idx, den_idx;
    const struct csfg_coeff* c;

    CSFG_DEBUG_ASSERT(vec_count(rational->num) == 0);
    CSFG_DEBUG_ASSERT(vec_count(rational->den) == 0);

    rc = csfg_expr_to_rational(
        in_pool, in_expr, variable, rational_pool, rational);
    if (rc != 0)
        return -1;

    /* Find the highest, non-zero coefficients */
    vec_enumerate_r (rational->num, num_idx, c)
        if (c->factor != 0.0)
            break;
    vec_enumerate_r (rational->den, den_idx, c)
        if (c->factor != 0.0)
            break;

    if (den_idx < 0 && num_idx < 0) /* 0/0 */
    {
        csfg_rational_clear(rational);
        csfg_poly_push(&rational->num, csfg_coeff(0.0, -1));
        csfg_poly_push(&rational->den, csfg_coeff(0.0, -1));
    }
    else if (den_idx < 0) /* Denominator has factor 0 */
    {
        double factor =
            vec_get(rational->num, num_idx)->factor < 0.0 ? -1.0 : 1.0;
        int inf = csfg_expr_inf(rational_pool);
        if (inf < 0)
            return -1;
        csfg_rational_clear(rational);
        csfg_poly_push(&rational->num, csfg_coeff(factor, inf));
        csfg_poly_push(&rational->den, csfg_coeff(0.0, -1));
    }
    else if (num_idx < 0) /* Numerator has factor 0 */
    {
        csfg_rational_clear(rational);
        csfg_poly_push(&rational->num, csfg_coeff(0.0, -1));
        csfg_poly_push(&rational->den, csfg_coeff(1.0, -1));
    }
    else if (num_idx > den_idx) /* Numerator diverges to inf */
    {
        double factor =
            vec_get(rational->num, num_idx)->factor < 0.0 ? -1.0 : 1.0;
        int inf = csfg_expr_inf(rational_pool);
        if (inf < 0)
            return -1;
        csfg_rational_clear(rational);
        csfg_poly_push(&rational->num, csfg_coeff(factor, inf));
        csfg_poly_push(&rational->den, csfg_coeff(1.0, -1));
    }
    else if (num_idx < den_idx) /* Denominator diverges to inf */
    {
        csfg_rational_clear(rational);
        csfg_poly_push(&rational->num, csfg_coeff(0.0, -1));
        csfg_poly_push(&rational->den, csfg_coeff(1.0, -1));
    }
    else /* Convergent */
    {
        csfg_poly_swap_values(rational->num, 0, num_idx);
        csfg_poly_swap_values(rational->den, 0, den_idx);
        rational->num->count = 1;
        rational->den->count = 1;
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_to_rational_limits(
    const struct csfg_expr_pool* in_pool,
    int                          in_expr,
    const struct csfg_var_table* vt,
    struct csfg_expr_pool**      rational_pool,
    struct csfg_rational*        rational)
{
    int                          slot, tmp_expr;
    struct str*                  key;
    struct csfg_var_table_entry* entry;
    struct csfg_expr_pool*       tmp_pool;

    CSFG_DEBUG_ASSERT(vec_count(rational->num) == 0);
    CSFG_DEBUG_ASSERT(vec_count(rational->den) == 0);

    csfg_expr_pool_init(&tmp_pool);

    tmp_expr = -1;
    hmap_for_each (vt->map, slot, key, entry)
    {
        struct strview var_name = str_view(key);
        if (entry->pool->nodes[entry->expr].type != CSFG_EXPR_INF)
            continue;

        if (vec_count(rational->num) > 0)
        {
            tmp_expr =
                csfg_rational_to_expr(rational, *rational_pool, &tmp_pool);
            if (tmp_expr < 0)
                goto fail;
            csfg_rational_clear(rational);
            if (csfg_expr_to_rational_limit(
                    tmp_pool, tmp_expr, var_name, rational_pool, rational) != 0)
                goto fail;
        }
        else
        {
            if (csfg_expr_to_rational_limit(
                    in_pool, in_expr, var_name, rational_pool, rational) != 0)
                goto fail;
        }
    }

    csfg_expr_pool_deinit(tmp_pool);
    return 0;

fail:
    csfg_expr_pool_deinit(tmp_pool);
    return -1;
}

/* ------------------------------------------------------------------------- */
int csfg_poly_to_expr(
    const struct csfg_poly*      poly,
    const struct csfg_expr_pool* coeff_pool,
    struct csfg_expr_pool**      expr_pool)
{
    const struct csfg_coeff* c;
    int                      expr = -1;
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

/* ------------------------------------------------------------------------- */
int csfg_rational_to_expr(
    const struct csfg_rational*  rational,
    const struct csfg_expr_pool* rational_pool,
    struct csfg_expr_pool**      expr_pool)
{
    return csfg_expr_div(
        expr_pool,
        csfg_poly_to_expr(rational->num, rational_pool, expr_pool),
        csfg_poly_to_expr(rational->den, rational_pool, expr_pool));
}
