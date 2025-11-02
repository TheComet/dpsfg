#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/var_table.h"
#include "csfg/util/str.h"
#include <assert.h>
#include <math.h>

/* ------------------------------------------------------------------------- */
static double
eval(struct csfg_expr_pool* pool, int n, const struct csfg_var_table* vt)
{
    double child_result[2];

    switch (pool->nodes[n].type)
    {
        case CSFG_EXPR_GC: break;
        case CSFG_EXPR_LIT: return pool->nodes[n].value.lit;
        case CSFG_EXPR_VAR: {
            double result;
            if (pool->nodes[n].visited)
                return NAN;

            pool->nodes[n].visited = 1;
            result = csfg_var_table_eval(
                vt,
                strlist_view(pool->var_names, pool->nodes[n].value.var_idx));
            pool->nodes[n].visited = 0;

            return result;
        }
        case CSFG_EXPR_INF: return INFINITY;
        case CSFG_EXPR_NEG:
            child_result[0] = eval(pool, pool->nodes[n].child[0], vt);
            break;
        case CSFG_EXPR_ADD:
        case CSFG_EXPR_MUL:
        case CSFG_EXPR_POW:
            child_result[0] = eval(pool, pool->nodes[n].child[0], vt);
            child_result[1] = eval(pool, pool->nodes[n].child[1], vt);
            break;
    }

    switch (pool->nodes[n].type)
    {
        case CSFG_EXPR_GC:
        case CSFG_EXPR_LIT:
        case CSFG_EXPR_VAR:
        case CSFG_EXPR_INF: assert(0); break;
        case CSFG_EXPR_NEG: return -child_result[0];
        case CSFG_EXPR_ADD: return child_result[0] + child_result[1];
        case CSFG_EXPR_MUL: return child_result[0] * child_result[1];
        case CSFG_EXPR_POW: return pow(child_result[0], child_result[1]);
    }

    return NAN;
}

/* ------------------------------------------------------------------------- */
static void reset_visited(struct csfg_expr_pool* expr)
{
    int n;
    for (n = 0; n != expr->count; ++n)
        expr->nodes[n].visited = 0;
}

/* ------------------------------------------------------------------------- */
double csfg_expr_eval(
    struct csfg_expr_pool* pool, int root, const struct csfg_var_table* vt)
{
    if (pool == NULL || root == -1)
        return NAN;
    if (vt != NULL)
        reset_visited(pool);
    return eval(pool, root, vt);
}

/* ------------------------------------------------------------------------- */
static int has_any_op_as_parent(const struct csfg_expr_pool* pool, int n)
{
    int parent = csfg_expr_find_parent(pool, n);
    if (parent < 0)
        return 0;
    switch (pool->nodes[parent].type)
    {
        case CSFG_EXPR_GC: assert(0); break;

        case CSFG_EXPR_LIT:
        case CSFG_EXPR_VAR:
        case CSFG_EXPR_INF: break;

        case CSFG_EXPR_NEG:
        case CSFG_EXPR_ADD:
        case CSFG_EXPR_MUL:
        case CSFG_EXPR_POW: return 1;
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
static int
has_parent_op_stronger_than_add(const struct csfg_expr_pool* pool, int n)
{
    int parent = csfg_expr_find_parent(pool, n);
    if (parent < 0)
        return 0;
    return pool->nodes[parent].type == CSFG_EXPR_MUL ||
           pool->nodes[parent].type == CSFG_EXPR_POW ||
           pool->nodes[parent].type == CSFG_EXPR_NEG;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_to_str(
    struct str** str, const struct csfg_expr_pool* pool, int expr)
{
    int left = pool->nodes[expr].child[0];
    int right = pool->nodes[expr].child[1];

    switch (pool->nodes[expr].type)
    {
        case CSFG_EXPR_GC: break;
        case CSFG_EXPR_LIT:
            if (isinf(pool->nodes[expr].value.lit))
            {
                if (str_append_cstr(str, "oo") != 0)
                    return -1;
            }
            else
            {
                if (str_append_int(str, pool->nodes[expr].value.lit) != 0)
                    return -1;
            }
            break;
        case CSFG_EXPR_VAR: {
            int         var_idx = pool->nodes[expr].value.var_idx;
            const char* var_name = strlist_cstr(pool->var_names, var_idx);
            if (str_append_cstr(str, var_name) != 0)
                return -1;
            break;
        }
        case CSFG_EXPR_INF:
            if (str_append_cstr(str, "oo") != 0)
                return -1;
            break;
        case CSFG_EXPR_NEG: {
            int need_parens = has_parent_op_stronger_than_add(pool, expr);
            if (str_append_char(str, '-') != 0)
                return -1;
            if (need_parens)
                if (str_append_char(str, '(') != 0)
                    return -1;
            if (csfg_expr_to_str(str, pool, left) != 0)
                return -1;
            if (need_parens)
                if (str_append_char(str, ')') != 0)
                    return -1;
            break;
        }
        case CSFG_EXPR_ADD: {
            int need_parens = has_parent_op_stronger_than_add(pool, expr);
            if (need_parens)
                if (str_append_char(str, '(') != 0)
                    return -1;
            if (csfg_expr_to_str(str, pool, left) != 0)
                return -1;
            if (str_append_cstr(str, " + ") != 0)
                return -1;
            if (csfg_expr_to_str(str, pool, right) != 0)
                return -1;
            if (need_parens)
                if (str_append_char(str, ')') != 0)
                    return -1;
            break;
        }
        case CSFG_EXPR_MUL: {
            if (csfg_expr_to_str(str, pool, left) != 0)
                return -1;
            if (str_append_cstr(str, "*") != 0)
                return -1;
            if (csfg_expr_to_str(str, pool, right) != 0)
                return -1;
            break;
        }
        case CSFG_EXPR_POW:
            if (str_append_cstr(str, "(") != 0)
                return -1;
            if (csfg_expr_to_str(str, pool, left) != 0)
                return -1;
            if (str_append_cstr(str, "^") != 0)
                return -1;
            if (csfg_expr_to_str(str, pool, right) != 0)
                return -1;
            if (str_append_cstr(str, ")") != 0)
                return -1;
            break;
    }

    return 0;
}
