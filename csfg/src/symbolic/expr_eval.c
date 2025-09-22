#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/var_table.h"
#include <assert.h>
#include <math.h>

/* ------------------------------------------------------------------------- */
static double
eval(struct csfg_expr_pool* expr, int n, const struct csfg_var_table* vt)
{
    double child_result[2];

    switch (expr->nodes[n].type)
    {
        case CSFG_EXPR_LIT: return expr->nodes[n].value.lit;
        case CSFG_EXPR_VAR: {
            if (expr->nodes[n].visited)
                return NAN;
            expr->nodes[n].visited = 1;

            return csfg_var_table_eval(
                vt,
                strlist_view(expr->var_names, expr->nodes[n].value.var_idx));
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
