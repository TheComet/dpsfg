#include "csfg/symbolic/expr.h"
#include "csfg/util/mem.h"
#include <stddef.h>

VEC_DEFINE(csfg_expr_vec, int, 8)

/* ------------------------------------------------------------------------- */
void csfg_expr_pool_init(struct csfg_expr_pool** pool)
{
    *pool = NULL;
}

/* ------------------------------------------------------------------------- */
void csfg_expr_pool_deinit(struct csfg_expr_pool* pool)
{
    if (pool != NULL)
    {
        strlist_deinit(pool->var_names);
        mem_free(pool);
    }
}

/* ------------------------------------------------------------------------- */
void csfg_expr_pool_clear(struct csfg_expr_pool* pool)
{
    pool->count = 0;
    strlist_clear(pool->var_names);
}

/* ------------------------------------------------------------------------- */
static void expr_init(struct csfg_expr_pool* pool, int capacity)
{
    pool->count = 0;
    pool->capacity = capacity;
    strlist_init(&pool->var_names);
}
static int new_expr(struct csfg_expr_pool** pool, enum csfg_expr_type type)
{
    int                    n;
    struct csfg_expr_pool* new_expr;

    if (*pool == NULL || (*pool)->count == (*pool)->capacity)
    {
        int header, data, new_cap;
        new_cap = *pool ? (*pool)->capacity * 2 : 16;
        header = offsetof(struct csfg_expr_pool, nodes);
        data = sizeof(struct csfg_expr_pool) * new_cap;
        new_expr = mem_realloc(*pool, header + data);
        if (new_expr == NULL)
            return -1;
        if (*pool == NULL)
            expr_init(new_expr, new_cap);
        *pool = new_expr;
    }

    n = (*pool)->count++;
    (*pool)->nodes[n].type = type;
    (*pool)->nodes[n].parent = -1;
    (*pool)->nodes[n].child[0] = -1;
    (*pool)->nodes[n].child[1] = -1;

    return n;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_lit(struct csfg_expr_pool** pool, double value)
{
    int n = new_expr(pool, CSFG_EXPR_LIT);
    if (n == -1)
        return -1;

    (*pool)->nodes[n].value.lit = value;

    return n;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_var(struct csfg_expr_pool** pool, struct strview name)
{
    int n = new_expr(pool, CSFG_EXPR_VAR);
    if (n == -1)
        return -1;

    (*pool)->nodes[n].value.var_idx = strlist_count((*pool)->var_names);
    if (strlist_add_view(&(*pool)->var_names, name) != 0)
        return -1;

    return n;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_binop(
    struct csfg_expr_pool** pool, enum csfg_expr_type type, int left, int right)
{
    int n = new_expr(pool, type);
    if (n == -1)
        return -1;

    (*pool)->nodes[n].child[0] = left;
    (*pool)->nodes[n].child[1] = right;
    (*pool)->nodes[left].parent = n;
    (*pool)->nodes[right].parent = n;

    return n;
}
int csfg_expr_add(struct csfg_expr_pool** pool, int left, int right)
{
    return csfg_expr_binop(pool, CSFG_EXPR_OP_ADD, left, right);
}
int csfg_expr_sub(struct csfg_expr_pool** pool, int left, int right)
{
    return csfg_expr_binop(pool, CSFG_EXPR_OP_SUB, left, right);
}
int csfg_expr_mul(struct csfg_expr_pool** pool, int left, int right)
{
    return csfg_expr_binop(pool, CSFG_EXPR_OP_MUL, left, right);
}
int csfg_expr_div(struct csfg_expr_pool** pool, int left, int right)
{
    return csfg_expr_binop(pool, CSFG_EXPR_OP_DIV, left, right);
}
int csfg_expr_pow(struct csfg_expr_pool** pool, int base, int exp)
{
    return csfg_expr_binop(pool, CSFG_EXPR_OP_POW, base, exp);
}
