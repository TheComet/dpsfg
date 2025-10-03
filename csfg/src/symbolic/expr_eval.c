#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/var_table.h"
#include "csfg/util/str.h"
#include <assert.h>
#include <math.h>

/* ------------------------------------------------------------------------- */
static double
eval(struct csfg_expr_pool* expr, int n, const struct csfg_var_table* vt)
{
    double child_result[2];

    switch (expr->nodes[n].type)
    {
        case CSFG_EXPR_GC: break;
        case CSFG_EXPR_LIT: return expr->nodes[n].value.lit;
        case CSFG_EXPR_VAR: {
            if (expr->nodes[n].visited)
                return NAN;
            expr->nodes[n].visited = 1;

            return csfg_var_table_eval(
                vt,
                strlist_cstr(expr->var_names, expr->nodes[n].value.var_idx));
        }
        case CSFG_EXPR_INF: return INFINITY;
        case CSFG_EXPR_NEG:
            child_result[0] = eval(expr, expr->nodes[n].child[0], vt);
            break;
        case CSFG_EXPR_OP_ADD:
        case CSFG_EXPR_OP_MUL:
        case CSFG_EXPR_OP_POW:
            child_result[0] = eval(expr, expr->nodes[n].child[0], vt);
            child_result[1] = eval(expr, expr->nodes[n].child[1], vt);
            break;
    }

    switch (expr->nodes[n].type)
    {
        case CSFG_EXPR_GC:
        case CSFG_EXPR_LIT:
        case CSFG_EXPR_VAR:
        case CSFG_EXPR_INF: assert(0); break;
        case CSFG_EXPR_NEG: return -child_result[0];
        case CSFG_EXPR_OP_ADD: return child_result[0] + child_result[1];
        case CSFG_EXPR_OP_MUL: return child_result[0] * child_result[1];
        case CSFG_EXPR_OP_POW: return pow(child_result[0], child_result[1]);
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
    struct csfg_expr_pool* expr, int root, const struct csfg_var_table* vt)
{
    if (expr == NULL || root == -1)
        return NAN;
    if (vt != NULL)
        reset_visited(expr);
    return eval(expr, root, vt);
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
            if (str_append_int(str, pool->nodes[expr].value.lit) != 0)
                return -1;
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
        case CSFG_EXPR_NEG:
            if (str_append_cstr(str, "-(") != 0)
                return -1;
            if (csfg_expr_to_str(str, pool, left) != 0)
                return -1;
            if (str_append_cstr(str, ")") != 0)
                return -1;
            break;
        case CSFG_EXPR_OP_ADD:
            if (str_append_cstr(str, "(") != 0)
                return -1;
            if (csfg_expr_to_str(str, pool, left) != 0)
                return -1;
            if (str_append_cstr(str, " + ") != 0)
                return -1;
            if (csfg_expr_to_str(str, pool, right) != 0)
                return -1;
            if (str_append_cstr(str, ")") != 0)
                return -1;
            break;
        case CSFG_EXPR_OP_MUL:
            if (str_append_cstr(str, "(") != 0)
                return -1;
            if (csfg_expr_to_str(str, pool, left) != 0)
                return -1;
            if (str_append_cstr(str, "*") != 0)
                return -1;
            if (csfg_expr_to_str(str, pool, right) != 0)
                return -1;
            if (str_append_cstr(str, ")") != 0)
                return -1;
            break;
        case CSFG_EXPR_OP_POW:
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
