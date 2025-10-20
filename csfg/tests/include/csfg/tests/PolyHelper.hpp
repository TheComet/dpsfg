#pragma once

#include <iostream>

extern "C" {
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/poly.h"
#include "csfg/util/str.h"
}

struct PolyHelper
{
    bool NodeEq(const struct csfg_expr_pool* pool, int n, const char* expr_str)
    {
        struct csfg_expr_pool* compare_pool;
        csfg_expr_pool_init(&compare_pool);
        int compare_expr = csfg_expr_parse(&compare_pool, cstr_view(expr_str));
        if (compare_expr < 0)
            return false;

        bool result = csfg_expr_equal(pool, n, compare_pool, compare_expr);
        if (!result)
        {
            struct str* str;
            str_init(&str);
            csfg_expr_to_str(&str, compare_pool, compare_expr);
            std::cerr << "Expected: " << str_cstr(str) << std::endl;
            str_clear(str);
            csfg_expr_to_str(&str, pool, n);
            std::cerr << "Actual: " << str_cstr(str) << std::endl;
            str_deinit(str);
        }

        csfg_expr_pool_deinit(compare_pool);
        return result;
    }

    bool NodeEq(const struct csfg_expr_pool* pool, int n, double value)
    {
        if (pool->nodes[n].type != CSFG_EXPR_LIT)
            return false;

        return NodeEq(pool, n, CSFG_EXPR_LIT) &&
               pool->nodes[n].value.lit == value;
    }

    bool
    NodeEq(const struct csfg_expr_pool* pool, int n, enum csfg_expr_type type)
    {
        return pool->nodes[n].type == type;
    }

    bool CoeffEq(
        const struct csfg_expr_pool* pool,
        const struct csfg_poly*      p,
        int                          idx,
        double                       factor)
    {
        if (idx >= vec_count(p))
        {
            std::cerr << "idx: " << idx << " is smaller than " << vec_count(p)
                      << std::endl;
            return false;
        }
        if (vec_get(p, idx)->expr != -1)
        {
            struct str* str;
            str_init(&str);
            csfg_expr_to_str(&str, pool, vec_get(p, idx)->expr);
            std::cerr << "idx: " << idx
                      << " expr expected to be -1, but it is \""
                      << str_cstr(str) << "\"" << std::endl;
            str_deinit(str);
            return false;
        }
        if (vec_get(p, idx)->factor != factor)
        {
            std::cerr << vec_get(p, idx)->factor << " != " << factor
                      << std::endl;
            return false;
        }
        return true;
    }

    bool CoeffEq(
        const struct csfg_expr_pool* pool,
        const struct csfg_poly*      p,
        int                          idx,
        double                       factor,
        const char*                  expr_str)
    {
        if (idx >= vec_count(p))
            return false;
        if (vec_get(p, idx)->expr < 0)
            return false;
        if (!NodeEq(pool, vec_get(p, idx)->expr, expr_str))
            return false;
        return vec_get(p, idx)->factor == factor;
    }

    bool CoeffEq(
        const struct csfg_expr_pool* pool,
        const struct csfg_poly*      p,
        int                          idx,
        double                       factor,
        enum csfg_expr_type          type)
    {
        if (idx >= vec_count(p))
            return false;
        if (vec_get(p, idx)->expr < 0)
            return false;
        if (!NodeEq(pool, vec_get(p, idx)->expr, type))
            return false;
        return vec_get(p, idx)->factor == factor;
    }
};
