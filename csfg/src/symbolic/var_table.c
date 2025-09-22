#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/var_table.h"
#include "csfg/util/hmap_str.h"
#include <math.h>

struct entry
{
    struct csfg_expr_pool* pool;
    int                    root;
};

HMAP_DECLARE_STR(static, csfg_var_hmap, struct entry, 16)
HMAP_DEFINE_STR(static, csfg_var_hmap, struct entry, 16)

/* ------------------------------------------------------------------------- */
void csfg_var_table_init(struct csfg_var_table* vt)
{
    csfg_expr_pool_init(&vt->pool);
    csfg_var_hmap_init(&vt->map);
}

/* ------------------------------------------------------------------------- */
void csfg_var_table_deinit(struct csfg_var_table* vt)
{
    int16_t       slot;
    struct str*   name;
    struct entry* entry;

    hmap_for_each (vt->map, slot, name, entry)
        csfg_expr_pool_deinit(entry->pool);
    csfg_var_hmap_deinit(vt->map);

    csfg_expr_pool_deinit(vt->pool);
}

/* ------------------------------------------------------------------------- */
static int var_table_populate(
    struct csfg_var_table*       vt,
    const struct csfg_expr_pool* pool,
    int                          parent,
    int                          n)
{
    double default_value = 0;

    if (n == -1)
        return 0;

    if (var_table_populate(vt, pool, n, pool->nodes[n].child[0]) != 0)
        return -1;
    if (var_table_populate(vt, pool, n, pool->nodes[n].child[1]) != 0)
        return -1;

    if (pool->nodes[n].type != CSFG_EXPR_VAR)
        return 0;
    if (parent != -1)
    {
        // If we are the right hand side of an expression that is mul or pow,
        // default value should be 1
        if (pool->nodes[parent].type == CSFG_EXPR_OP_MUL &&
            pool->nodes[parent].child[1] == n)
        {
            default_value = 1;
        }
        if (pool->nodes[parent].type == CSFG_EXPR_OP_POW &&
            pool->nodes[parent].child[1] == n)
        {
            default_value = 1;
        }
    }

    return csfg_var_table_set_lit(
        vt,
        strlist_view(pool->var_names, pool->nodes[n].value.var_idx),
        default_value);
}
int csfg_var_table_populate(
    struct csfg_var_table* vt, const struct csfg_expr_pool* pool, int n)
{
    int    parent;
    double default_value = 0;

    if (pool == NULL)
        return 0;
    if (n == -1)
        return 0;

    return var_table_populate(vt, pool, -1, n);
}

/* ------------------------------------------------------------------------- */
int csfg_var_table_set_lit(
    struct csfg_var_table* vt, struct strview name, double value)
{
    struct entry* entry;
    int           n;

    switch (csfg_var_hmap_emplace_or_get(&vt->map, name, &entry))
    {
        case HMAP_OOM: return -1;
        case HMAP_NEW: csfg_expr_pool_init(&entry->pool); break;
        case HMAP_EXISTS: csfg_expr_pool_clear(entry->pool); break;
    }

    n = csfg_expr_lit(&entry->pool, value);
    if (n == -1)
        return -1;
    entry->root = n;
    return 0;
}

/* ------------------------------------------------------------------------- */
int csfg_var_table_set_expr(
    struct csfg_var_table* vt,
    struct strview         name,
    struct csfg_expr_pool* pool,
    int                    root)
{
    struct entry* entry;
    int           n;

    switch (csfg_var_hmap_emplace_or_get(&vt->map, name, &entry))
    {
        case HMAP_OOM: return -1;
        case HMAP_EXISTS: csfg_expr_pool_deinit(entry->pool);
        case HMAP_NEW: break;
    }

    entry->pool = pool;
    entry->root = root;
    return 0;
}

/* ------------------------------------------------------------------------- */
struct csfg_expr_pool* csfg_var_table_get(
    const struct csfg_var_table* vt, struct strview name, int* root)
{
    struct entry* entry = csfg_var_hmap_find(vt->map, name);
    if (entry == NULL)
        return NULL;
    *root = entry->root;
    return entry->pool;
}

/* ------------------------------------------------------------------------- */
double csfg_var_table_eval(const struct csfg_var_table* vt, struct strview name)
{
    int                    root;
    struct csfg_expr_pool* pool;
    assert(vt != NULL);

    pool = csfg_var_table_get(vt, name, &root);
    if (pool == NULL)
        return NAN;

    return csfg_expr_eval(pool, root, vt);
}
