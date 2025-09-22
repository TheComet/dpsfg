#include "csfg/symbolic/expr.h"
#include "csfg/util/mem.h"
#include <assert.h>
#include <stddef.h>

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
    (*pool)->nodes[n].child[0] = -1;
    (*pool)->nodes[n].child[1] = -1;
    (*pool)->nodes[n].visited = 0;

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
int csfg_expr_inf(struct csfg_expr_pool** pool)
{
    return new_expr(pool, CSFG_EXPR_INF);
}

/* ------------------------------------------------------------------------- */
int csfg_expr_neg(struct csfg_expr_pool** pool, int child)
{
    int n = new_expr(pool, CSFG_EXPR_NEG);
    if (n == -1 || child == -1)
        return -1;

    (*pool)->nodes[n].child[0] = child;

    return n;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_set_lit(struct csfg_expr_pool** pool, int n, double value)
{
    if (n == -1)
        return -1;

    (*pool)->nodes[n].type = CSFG_EXPR_LIT;
    (*pool)->nodes[n].child[0] = -1;
    (*pool)->nodes[n].child[1] = -1;
    (*pool)->nodes[n].value.lit = value;

    return n;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_binop(
    struct csfg_expr_pool** pool, enum csfg_expr_type type, int left, int right)
{
    return csfg_expr_set_binop(pool, new_expr(pool, type), type, left, right);
}
/* clang-format off */
int csfg_expr_add(struct csfg_expr_pool** pool, int left, int right)
{return csfg_expr_binop(pool, CSFG_EXPR_OP_ADD, left, right);}
int csfg_expr_mul(struct csfg_expr_pool** pool, int left, int right)
{return csfg_expr_binop(pool, CSFG_EXPR_OP_MUL, left, right);}
int csfg_expr_pow(struct csfg_expr_pool** pool, int base, int exp)
{return csfg_expr_binop(pool, CSFG_EXPR_OP_POW, base, exp);}
/* clang-format on */
int csfg_expr_sub(struct csfg_expr_pool** pool, int left, int right)
{
    /* l + -r */
    return csfg_expr_add(pool, left, csfg_expr_neg(pool, right));
}
int csfg_expr_div(struct csfg_expr_pool** pool, int left, int right)
{
    /* l * r^-1 */
    return csfg_expr_mul(
        pool, left, csfg_expr_pow(pool, right, csfg_expr_lit(pool, -1)));
}

/* ------------------------------------------------------------------------- */
int csfg_expr_set_binop(
    struct csfg_expr_pool** pool,
    int                     n,
    enum csfg_expr_type     type,
    int                     left,
    int                     right)
{
    if (n == -1 || left == -1 || right == -1)
        return -1;

    (*pool)->nodes[n].type = type;
    (*pool)->nodes[n].child[0] = left;
    (*pool)->nodes[n].child[1] = right;

    return n;
}
/* clang-format off */
int csfg_expr_set_add(struct csfg_expr_pool** pool, int n, int left, int right)
{return csfg_expr_set_binop(pool, n, CSFG_EXPR_OP_ADD, left, right);}
int csfg_expr_set_mul(struct csfg_expr_pool** pool, int n, int left, int right)
{return csfg_expr_set_binop(pool, n, CSFG_EXPR_OP_MUL, left, right);}
int csfg_expr_set_pow(struct csfg_expr_pool** pool, int n, int base, int exp)
{return csfg_expr_set_binop(pool, n, CSFG_EXPR_OP_POW, base, exp);}
/* clang-format on */

/* ------------------------------------------------------------------------- */
int csfg_expr_find_parent(const struct csfg_expr_pool* pool, int n)
{
    int p;
    for (p = 0; p != pool->count; ++p)
        if (pool->nodes[p].child[0] == n || pool->nodes[p].child[1] == n)
            return p;
    return -1;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_find_top_of_chain(const struct csfg_expr_pool* pool, int n)
{
    enum csfg_expr_type type = pool->nodes[n].type;
    while (1)
    {
        int parent = csfg_expr_find_parent(pool, n);
        if (parent == -1 || pool->nodes[parent].type != type)
            return n;
        n = parent;
    }
}

/* ------------------------------------------------------------------------- */
void csfg_expr_mark_deleted(struct csfg_expr_pool* pool, int n)
{
    pool->nodes[n].type = CSFG_EXPR_GC;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_gc(struct csfg_expr_pool* pool, int root)
{
    int n;
    for (n = 0; n < pool->count; ++n)
    {
        int parent, end = pool->count - 1;
        if (pool->nodes[n].type != CSFG_EXPR_GC)
            continue;

        parent = csfg_expr_find_parent(pool, end);
        if (parent != -1)
        {
            if (pool->nodes[parent].child[0] == end)
                pool->nodes[parent].child[0] = n;
            if (pool->nodes[parent].child[1] == end)
                pool->nodes[parent].child[1] = n;
        }
        pool->nodes[n] = pool->nodes[end];
        pool->count--;

        if (root == end)
            root = n;
        n--;
    }

    return root;
}

/* ------------------------------------------------------------------------- */
void csfg_expr_collapse_into_parent(
    struct csfg_expr_pool* pool, int child, int parent)
{
    int dangling_child = pool->nodes[parent].child[0] == child
                             ? pool->nodes[parent].child[1]
                             : pool->nodes[parent].child[0];
    assert(
        pool->nodes[parent].child[0] == child ||
        pool->nodes[parent].child[1] == child);

    pool->nodes[parent] = pool->nodes[child];

    csfg_expr_mark_deleted(pool, child);
    csfg_expr_mark_deleted(pool, dangling_child);
}
