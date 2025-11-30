#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/var_table.h"
#include <math.h>

HMAP_DEFINE_STR(extern, csfg_var_hmap, struct csfg_var_table_entry, 16)

/* -------------------------------------------------------------------------- */
void csfg_var_table_init(struct csfg_var_table* vt)
{
    csfg_var_hmap_init(&vt->map);
}

/* -------------------------------------------------------------------------- */
void csfg_var_table_deinit(struct csfg_var_table* vt)
{
    int16_t                      slot;
    struct str*                  name;
    struct csfg_var_table_entry* entry;

    hmap_for_each (vt->map, slot, name, entry)
        (void)name, csfg_expr_pool_deinit(entry->pool);
    csfg_var_hmap_deinit(vt->map);
}

/* -------------------------------------------------------------------------- */
void csfg_var_table_clear(struct csfg_var_table* vt)
{
    int16_t                      slot;
    struct str*                  name;
    struct csfg_var_table_entry* entry;

    hmap_for_each (vt->map, slot, name, entry)
        (void)name, csfg_expr_pool_deinit(entry->pool);
    csfg_var_hmap_clear(vt->map);
}

/* -------------------------------------------------------------------------- */
static int var_table_populate(
    struct csfg_var_table* vt, const struct csfg_expr_pool* pool, int n)
{
    struct csfg_var_table_entry* entry;
    struct strview               name;
    const double                 default_value = 1.0;

    if (n == -1)
        return 0;

    if (var_table_populate(vt, pool, pool->nodes[n].child[0]) != 0)
        return -1;
    if (var_table_populate(vt, pool, pool->nodes[n].child[1]) != 0)
        return -1;

    if (pool->nodes[n].type != CSFG_EXPR_VAR)
        return 0;

    name = strlist_view(pool->var_names, pool->nodes[n].value.var_idx);
    switch (csfg_var_hmap_emplace_or_get(&vt->map, name, &entry))
    {
        case HMAP_OOM: return -1;
        case HMAP_EXISTS:
            entry->visited = 1;

            /* If the user has tweaked this value, leave it as-is */
            CSFG_DEBUG_ASSERT(
                entry->pool->nodes[entry->expr].type == CSFG_EXPR_LIT);
            if (entry->pool->nodes[entry->expr].value.lit != default_value)
                return 0;

            /* Otherwise set it to the default value */
            csfg_expr_pool_deinit(entry->pool);
            /* fallthrough */
        case HMAP_NEW:
            csfg_expr_pool_init(&entry->pool);
            entry->visited = 1;
            entry->expr = csfg_expr_lit(&entry->pool, default_value);
            if (entry->expr < 0)
            {
                csfg_expr_pool_deinit(entry->pool);
                csfg_var_hmap_erase(vt->map, name);
                return -1;
            }
            break;
    }

    return 0;
}
int csfg_var_table_populate(
    struct csfg_var_table* vt, const struct csfg_expr_pool* pool, int n)
{
    if (pool == NULL)
        return 0;
    if (n == -1)
        return 0;

    if (var_table_populate(vt, pool, n) != 0)
        return -1;

    return 0;
}

/* -------------------------------------------------------------------------- */
void csfg_var_table_reset_visited(struct csfg_var_table* vt)
{
    int                          slot;
    const struct str*            name;
    struct csfg_var_table_entry* entry;

    hmap_for_each (vt->map, slot, name, entry)
        (void)slot, (void)name, entry->visited = 0;
}

/* -------------------------------------------------------------------------- */
void csfg_var_table_erase_unvisited(struct csfg_var_table* vt)
{
    int                          slot;
    const struct str*            name;
    struct csfg_var_table_entry* entry;

    hmap_for_each (vt->map, slot, name, entry)
        if (!entry->visited)
        {
            csfg_expr_pool_deinit(entry->pool);
            (void)name, csfg_var_hmap_erase_slot(vt->map, slot);
        }
}

/* -------------------------------------------------------------------------- */
int csfg_var_table_set_lit(
    struct csfg_var_table* vt, struct strview name, double value)
{
    struct csfg_var_table_entry* entry;
    int                          n;

    switch (csfg_var_hmap_emplace_or_get(&vt->map, name, &entry))
    {
        case HMAP_OOM: return -1;
        case HMAP_NEW: csfg_expr_pool_init(&entry->pool); break;
        case HMAP_EXISTS: csfg_expr_pool_clear(entry->pool); break;
    }

    n = csfg_expr_lit(&entry->pool, value);
    if (n == -1)
    {
        csfg_expr_pool_deinit(entry->pool);
        csfg_var_hmap_erase(vt->map, name);
        return -1;
    }
    entry->expr = n;
    return 0;
}

/* -------------------------------------------------------------------------- */
int csfg_var_table_set_expr(
    struct csfg_var_table* vt,
    struct strview         name,
    struct csfg_expr_pool* pool,
    int                    expr)
{
    struct csfg_var_table_entry* entry;

    switch (csfg_var_hmap_emplace_or_get(&vt->map, name, &entry))
    {
        case HMAP_OOM: return -1;
        case HMAP_EXISTS: csfg_expr_pool_deinit(entry->pool);
        case HMAP_NEW: break;
    }

    entry->pool = pool;
    entry->expr = expr;
    return 0;
}

/* -------------------------------------------------------------------------- */
int csfg_var_table_set_parse_expr(
    struct csfg_var_table* vt, struct strview name, struct strview str)
{
    int                    expr;
    struct csfg_expr_pool* pool;

    csfg_expr_pool_init(&pool);
    expr = csfg_expr_parse(&pool, str);
    if (expr < 0)
        goto fail;
    if (csfg_var_table_set_expr(vt, name, pool, expr) != 0)
        goto fail;

    return 0;

fail:
    csfg_expr_pool_deinit(pool);
    return -1;
}

/* -------------------------------------------------------------------------- */
struct csfg_expr_pool* csfg_var_table_get(
    const struct csfg_var_table* vt, struct strview name, int* expr)
{
    struct csfg_var_table_entry* entry = csfg_var_hmap_find(vt->map, name);
    if (entry == NULL)
        return NULL;
    *expr = entry->expr;
    return entry->pool;
}

/* -------------------------------------------------------------------------- */
double csfg_var_table_eval(const struct csfg_var_table* vt, struct strview name)
{
    int                    expr;
    struct csfg_expr_pool* pool;
    assert(vt != NULL);

    pool = csfg_var_table_get(vt, name, &expr);
    if (pool == NULL)
        return NAN;

    return csfg_expr_eval(pool, expr, vt);
}
