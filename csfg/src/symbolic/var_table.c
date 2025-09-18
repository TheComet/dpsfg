#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/var_table.h"
#include "csfg/util/hmap_str.h"
#include <math.h>

HMAP_DECLARE_STR(static, csfg_var_hmap, struct csfg_expr*, 16)
HMAP_DEFINE_STR(static, csfg_var_hmap, struct csfg_expr*, 16)

struct csfg_var_table
{
    struct csfg_var_hmap map;
};

/* ------------------------------------------------------------------------- */
void csfg_var_table_init(struct csfg_var_table** vt)
{
    csfg_var_hmap_init((struct csfg_var_hmap**)vt);
}

/* ------------------------------------------------------------------------- */
void csfg_var_table_deinit(struct csfg_var_table* vt)
{
    int16_t            slot;
    struct str*        name;
    struct csfg_expr** expr;

    hmap_for_each (&vt->map, slot, name, expr)
        csfg_expr_deinit(*expr);
    csfg_var_hmap_deinit(&vt->map);
}

/* ------------------------------------------------------------------------- */
int var_table_populate(
    struct csfg_var_table** vt, const struct csfg_expr* expr, int n)
{
    int    parent;
    double default_value = 0;

    if (n == -1)
        return 0;

    if (var_table_populate(vt, expr, expr->nodes[n].child[0]) != 0)
        return -1;
    if (var_table_populate(vt, expr, expr->nodes[n].child[1]) != 0)
        return -1;

    if (expr->nodes[n].type != CSFG_EXPR_VAR)
        return 0;
    parent = expr->nodes[n].parent;
    if (parent != -1)
    {
        // If we are the right hand side of an expression that is mul or pow,
        // default value should be 1
        if (expr->nodes[parent].type == CSFG_EXPR_OP_MUL &&
            expr->nodes[parent].child[1] == n)
        {
            default_value = 1;
        }
        if (expr->nodes[parent].type == CSFG_EXPR_OP_POW &&
            expr->nodes[parent].child[1] == n)
        {
            default_value = 1;
        }
    }

    return csfg_var_table_set_lit(
        vt,
        strlist_view(expr->var_names, expr->nodes[n].value.var_idx),
        default_value);
}
int csfg_var_table_populate(
    struct csfg_var_table** vt, const struct csfg_expr* expr)
{
    if (expr == NULL)
        return 0;
    return var_table_populate(vt, expr, expr->root);
}

/* ------------------------------------------------------------------------- */
int csfg_var_table_set_lit(
    struct csfg_var_table** vt, struct strview name, double value)
{
    struct csfg_expr** expr;
    int                n;

    switch (
        csfg_var_hmap_emplace_or_get((struct csfg_var_hmap**)vt, name, &expr))
    {
        case HMAP_OOM: return -1;
        case HMAP_NEW: csfg_expr_init(expr); break;
        case HMAP_EXISTS: csfg_expr_clear(*expr); break;
    }

    n = csfg_expr_lit(expr, value);
    if (n == -1)
        return -1;
    (*expr)->root = n;
    return 0;
}

/* ------------------------------------------------------------------------- */
int csfg_var_table_set_expr(
    struct csfg_var_table** vt, struct strview name, struct csfg_expr* expr)
{
    struct csfg_expr** expr_entry;
    int                n;

    switch (csfg_var_hmap_emplace_or_get(
        (struct csfg_var_hmap**)vt, name, &expr_entry))
    {
        case HMAP_OOM: return -1;
        case HMAP_EXISTS: csfg_expr_deinit(*expr_entry);
        case HMAP_NEW: break;
    }

    *expr_entry = expr;
    return 0;
}

/* ------------------------------------------------------------------------- */
struct csfg_expr*
csfg_var_table_get(const struct csfg_var_table* vt, struct strview name)
{
    struct csfg_expr** expr = csfg_var_hmap_find(&vt->map, name);
    if (expr == NULL)
        return NULL;
    return *expr;
}

/* ------------------------------------------------------------------------- */
double csfg_var_table_eval(const struct csfg_var_table* vt, struct strview name)
{
    struct csfg_expr* var;
    assert(vt != NULL);

    var = csfg_var_table_get(vt, name);
    if (var == NULL)
        return NAN;

    return csfg_expr_eval(var, vt);
}
