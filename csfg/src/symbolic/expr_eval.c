#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/var_table.h"
#include <assert.h>
#include <math.h>

/* ------------------------------------------------------------------------- */
static double
eval(struct csfg_expr* expr, const struct csfg_var_table* vt, int n)
{
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
        case CSFG_EXPR_OP_ADD:
            return eval(expr, vt, expr->nodes[n].child[0]) +
                   eval(expr, vt, expr->nodes[n].child[1]);
        case CSFG_EXPR_OP_SUB:
            return eval(expr, vt, expr->nodes[n].child[0]) -
                   eval(expr, vt, expr->nodes[n].child[1]);
        case CSFG_EXPR_OP_MUL:
            return eval(expr, vt, expr->nodes[n].child[0]) *
                   eval(expr, vt, expr->nodes[n].child[1]);
        case CSFG_EXPR_OP_DIV:
            return eval(expr, vt, expr->nodes[n].child[0]) /
                   eval(expr, vt, expr->nodes[n].child[1]);
        case CSFG_EXPR_OP_POW:
            return pow(
                eval(expr, vt, expr->nodes[n].child[0]),
                eval(expr, vt, expr->nodes[n].child[1]));
    }
}

/* ------------------------------------------------------------------------- */
static void reset_visited(struct csfg_expr* expr)
{
    int n;
    for (n = 0; n != expr->count; ++n)
        expr->nodes[n].visited = 0;
}

/* ------------------------------------------------------------------------- */
double csfg_expr_eval(struct csfg_expr* expr, const struct csfg_var_table* vt)
{
    if (expr == NULL)
        return NAN;
    if (vt != NULL)
        reset_visited(expr);
    return eval(expr, vt, expr->root);
}
