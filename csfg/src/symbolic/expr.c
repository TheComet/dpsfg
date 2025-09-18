#include "csfg/symbolic/expr.h"
#include "csfg/util/mem.h"
#include "csfg/util/strspan.h"
#include <stddef.h>

/* ------------------------------------------------------------------------- */
void csfg_expr_init(struct csfg_expr** expr)
{
    *expr = NULL;
}

/* ------------------------------------------------------------------------- */
void csfg_expr_deinit(struct csfg_expr* expr)
{
    if (expr != NULL)
    {
        strlist_deinit(expr->var_names);
        mem_free(expr);
    }
}

/* ------------------------------------------------------------------------- */
void csfg_expr_clear(struct csfg_expr* expr)
{
    expr->count = 0;
    expr->root = -1;
}

/* ------------------------------------------------------------------------- */
static void expr_init(struct csfg_expr* expr, int capacity)
{
    expr->count = 0;
    expr->capacity = capacity;
    expr->root = -1;
    strlist_init(&expr->var_names);
}
static int new_expr(struct csfg_expr** expr, enum csfg_expr_type type)
{
    int               n;
    struct csfg_expr* new_expr;

    if (*expr == NULL || (*expr)->count == (*expr)->capacity)
    {
        int header, data, new_cap;
        new_cap = *expr ? (*expr)->capacity * 2 : 16;
        header = offsetof(struct csfg_expr, nodes);
        data = sizeof(struct csfg_expr) * new_cap;
        new_expr = mem_realloc(*expr, header + data);
        if (new_expr == NULL)
            return -1;
        if (*expr == NULL)
            expr_init(new_expr, new_cap);
        *expr = new_expr;
    }

    n = (*expr)->count++;
    (*expr)->nodes[n].type = type;
    (*expr)->nodes[n].parent = -1;
    (*expr)->nodes[n].child[0] = -1;
    (*expr)->nodes[n].child[1] = -1;

    return n;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_lit(struct csfg_expr** expr, double value)
{
    int n = new_expr(expr, CSFG_EXPR_LIT);
    if (n == -1)
        return -1;

    (*expr)->nodes[n].value.lit = value;

    return n;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_var(struct csfg_expr** expr, struct strview name)
{
    int n = new_expr(expr, CSFG_EXPR_VAR);
    if (n == -1)
        return -1;

    (*expr)->nodes[n].value.var_idx = strlist_count((*expr)->var_names);
    if (strlist_add_view(&(*expr)->var_names, name) != 0)
        return -1;

    return n;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_binop(
    struct csfg_expr** expr, enum csfg_expr_type type, int left, int right)
{
    int n = new_expr(expr, type);
    if (n == -1)
        return -1;

    (*expr)->nodes[n].child[0] = left;
    (*expr)->nodes[n].child[1] = right;
    (*expr)->nodes[left].parent = n;
    (*expr)->nodes[right].parent = n;

    return n;
}
int csfg_expr_add(struct csfg_expr** expr, int left, int right)
{
    return csfg_expr_binop(expr, CSFG_EXPR_OP_ADD, left, right);
}
int csfg_expr_sub(struct csfg_expr** expr, int left, int right)
{
    return csfg_expr_binop(expr, CSFG_EXPR_OP_SUB, left, right);
}
int csfg_expr_mul(struct csfg_expr** expr, int left, int right)
{
    return csfg_expr_binop(expr, CSFG_EXPR_OP_MUL, left, right);
}
int csfg_expr_div(struct csfg_expr** expr, int left, int right)
{
    return csfg_expr_binop(expr, CSFG_EXPR_OP_DIV, left, right);
}
int csfg_expr_pow(struct csfg_expr** expr, int base, int exp)
{
    return csfg_expr_binop(expr, CSFG_EXPR_OP_POW, base, exp);
}
