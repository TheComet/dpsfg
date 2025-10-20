#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/rational.h"
#include "csfg/symbolic/var_table.h"
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
    if (pool != NULL)
    {
        pool->count = 0;
        strlist_clear(pool->var_names);
    }
}

/* ------------------------------------------------------------------------- */
static void pool_init(struct csfg_expr_pool* pool)
{
    pool->count = 0;
    strlist_init(&pool->var_names);
}

/* ------------------------------------------------------------------------- */
int csfg_expr_new(
    struct csfg_expr_pool** pool, enum csfg_expr_type type, int left, int right)
{
    int                    n;
    struct csfg_expr_pool* new_pool;

    if (*pool == NULL || (*pool)->count == (*pool)->capacity)
    {
        int header, data, new_cap;
        new_cap = *pool ? (*pool)->capacity * 2 : 16;
        header = offsetof(struct csfg_expr_pool, nodes);
        data = sizeof(struct csfg_expr_node) * new_cap;
        new_pool = mem_realloc(*pool, header + data);
        if (new_pool == NULL)
            return -1;
        if (*pool == NULL)
            pool_init(new_pool);
        new_pool->capacity = new_cap;
        *pool = new_pool;
    }

    n = (*pool)->count++;
    (*pool)->nodes[n].type = type;
    (*pool)->nodes[n].child[0] = left;
    (*pool)->nodes[n].child[1] = right;
    (*pool)->nodes[n].visited = 0;

    return n;
}

/* ------------------------------------------------------------------------- */
static int insert_substitutions(
    struct csfg_expr_pool** pool, int n, const struct csfg_var_table* vt)
{
    int                                left, right, dup;
    const struct csfg_var_table_entry* entry;

    entry = NULL;
    if ((*pool)->nodes[n].type == CSFG_EXPR_VAR)
    {
        entry = csfg_var_hmap_find(
            vt->map,
            strlist_view((*pool)->var_names, (*pool)->nodes[n].value.var_idx));
    }

    if (entry != NULL)
    {
        if (entry->pool->nodes[entry->expr].type == CSFG_EXPR_INF)
            return 0;

        if (entry->pool->nodes[entry->expr].visited)
            return -1;
        entry->pool->nodes[entry->expr].visited = 1;

        dup = csfg_expr_dup_recurse_from(pool, entry->pool, entry->expr);
        if (dup == -1)
            return -1;
        (*pool)->nodes[n] = (*pool)->nodes[dup];
        csfg_expr_mark_deleted(*pool, dup);

        if (insert_substitutions(pool, n, vt) != 0)
            return -1;
    }

    left = (*pool)->nodes[n].child[0];
    right = (*pool)->nodes[n].child[1];
    if (left > -1)
        if (insert_substitutions(pool, left, vt) != 0)
            return -1;
    if (right > -1)
        if (insert_substitutions(pool, right, vt) != 0)
            return -1;

    if (entry != NULL)
        entry->pool->nodes[entry->expr].visited = 0;

    return 0;
}
int csfg_expr_insert_substitutions(
    struct csfg_expr_pool** pool, int expr, const struct csfg_var_table* vt)
{
    int                          slot;
    struct str*                  key;
    struct csfg_var_table_entry* entry;
    hmap_for_each (vt->map, slot, key, entry)
        (void)slot, (void)key, entry->pool->nodes[entry->expr].visited = 0;

    return insert_substitutions(pool, expr, vt);
}

/* ------------------------------------------------------------------------- */
int csfg_expr_apply_limits(
    const struct csfg_expr_pool* in_pool,
    int                          in_expr,
    const struct csfg_var_table* vt,
    struct csfg_expr_pool**      out_pool)
{
    int                          slot, out_expr, result;
    struct str*                  key;
    struct csfg_var_table_entry* entry;
    struct csfg_rational         rational;
    struct csfg_expr_pool*       tmp_pool;

    csfg_rational_init(&rational);
    csfg_expr_pool_init(&tmp_pool);

    out_expr = -1;
    hmap_for_each (vt->map, slot, key, entry)
    {
        struct strview var_name = str_view(key);
        if (entry->pool->nodes[entry->expr].type != CSFG_EXPR_INF)
            continue;

        csfg_expr_pool_clear(tmp_pool);
        csfg_rational_clear(&rational);
        result = csfg_expr_to_rational_limit(
            in_pool, in_expr, var_name, &tmp_pool, &rational);
        if (result != 0)
            goto fail;

        out_expr = csfg_rational_to_expr(&rational, tmp_pool, out_pool);
        if (out_expr < 0)
            goto fail;

        in_pool = *out_pool;
        in_expr = out_expr;
    }

    /* If no INF variables were set, then we just copy */
    if (out_expr < 0)
        out_expr = csfg_expr_dup_recurse_from(out_pool, in_pool, in_expr);

    csfg_expr_pool_deinit(tmp_pool);
    csfg_rational_deinit(&rational);
    return out_expr;

fail:
    csfg_expr_pool_deinit(tmp_pool);
    csfg_rational_deinit(&rational);
    return -1;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_lit(struct csfg_expr_pool** pool, double value)
{
    int n = csfg_expr_new(pool, CSFG_EXPR_LIT, -1, -1);
    if (n == -1)
        return -1;

    (*pool)->nodes[n].value.lit = value;

    return n;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_var(struct csfg_expr_pool** pool, struct strview name)
{
    int n = csfg_expr_new(pool, CSFG_EXPR_VAR, -1, -1);
    if (n == -1)
        return -1;

    if (csfg_expr_set_var(pool, n, name) == -1)
        return -1;

    return n;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_inf(struct csfg_expr_pool** pool)
{
    return csfg_expr_new(pool, CSFG_EXPR_INF, -1, -1);
}

/* ------------------------------------------------------------------------- */
int csfg_expr_neg(struct csfg_expr_pool** pool, int child)
{
    int n = csfg_expr_new(pool, CSFG_EXPR_NEG, child, -1);
    if (n == -1 || child == -1)
        return -1;

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
int csfg_expr_set_var(struct csfg_expr_pool** pool, int n, struct strview name)
{
    int            i;
    struct strview str;

    if (n == -1)
        return -1;

    strlist_for_each ((*pool)->var_names, i, str)
        if (strview_eq(str, name))
        {
            (*pool)->nodes[n].value.var_idx = i;
            return 0;
        }

    (*pool)->nodes[n].value.var_idx = strlist_count((*pool)->var_names);
    if (strlist_add_view(&(*pool)->var_names, name) != 0)
        return -1;

    return 0;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_set_inf(struct csfg_expr_pool** pool, int n)
{
    if (n == -1)
        return -1;

    (*pool)->nodes[n].type = CSFG_EXPR_INF;
    (*pool)->nodes[n].child[0] = -1;
    (*pool)->nodes[n].child[1] = -1;

    return n;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_set_neg(struct csfg_expr_pool** pool, int n, int child)
{
    if (n == -1 || child == -1)
        return -1;

    (*pool)->nodes[n].type = CSFG_EXPR_NEG;
    (*pool)->nodes[n].child[0] = child;
    (*pool)->nodes[n].child[1] = -1;

    return n;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_binop(
    struct csfg_expr_pool** pool, enum csfg_expr_type type, int left, int right)
{
    if (left == -1 || right == -1)
        return -1;
    return csfg_expr_new(pool, type, left, right);
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
int csfg_expr_dup_recurse_from(
    struct csfg_expr_pool** dst, const struct csfg_expr_pool* src, int n)
{
    int dup, left, right;
    if (n == -1)
        return -1;

    left = src->nodes[n].child[0];
    right = src->nodes[n].child[1];
    if (left != -1)
        if ((left = csfg_expr_dup_recurse_from(dst, src, left)) == -1)
            return -1;
    if (right != -1)
        if ((right = csfg_expr_dup_recurse_from(dst, src, right)) == -1)
            return -1;

    dup = csfg_expr_dup_single_from(dst, src, n);
    if (dup == -1)
        return -1;
    (*dst)->nodes[dup].child[0] = left;
    (*dst)->nodes[dup].child[1] = right;

    return dup;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_dup_recurse(struct csfg_expr_pool** pool, int n)
{
    int dup, left, right;
    if (n == -1)
        return -1;

    left = (*pool)->nodes[n].child[0];
    right = (*pool)->nodes[n].child[1];
    if (left != -1)
        if ((left = csfg_expr_dup_recurse(pool, left)) == -1)
            return -1;
    if (right != -1)
        if ((right = csfg_expr_dup_recurse(pool, right)) == -1)
            return -1;

    dup = csfg_expr_new(pool, (*pool)->nodes[n].type, left, right);
    if (dup == -1)
        return -1;

    (*pool)->nodes[dup].value = (*pool)->nodes[n].value;

    return dup;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_dup_single_from(
    struct csfg_expr_pool** dst, const struct csfg_expr_pool* src, int n)
{
    int dup;
    if (n == -1)
        return -1;
    dup = csfg_expr_new(dst, src->nodes[n].type, -1, -1);
    if (dup == -1)
        return -1;

    switch (src->nodes[n].type)
    {
        case CSFG_EXPR_GC: break;
        case CSFG_EXPR_LIT:
            (*dst)->nodes[dup].value = src->nodes[n].value;
            break;
        case CSFG_EXPR_VAR: {
            int            orig_idx = src->nodes[n].value.var_idx;
            struct strview orig = strlist_view(src->var_names, orig_idx);
            if (csfg_expr_set_var(dst, dup, orig) == -1)
                return -1;
            break;
        }
        case CSFG_EXPR_INF: break;
        case CSFG_EXPR_NEG: break;
        case CSFG_EXPR_OP_ADD: break;
        case CSFG_EXPR_OP_MUL: break;
        case CSFG_EXPR_OP_POW: break;
    }

    return dup;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_dup_single(struct csfg_expr_pool** pool, int n)
{
    int dup;
    if (n == -1)
        return -1;
    dup = csfg_expr_new(pool, CSFG_EXPR_GC, -1, -1);
    if (dup == -1)
        return -1;
    (*pool)->nodes[dup] = (*pool)->nodes[n];
    return dup;
}

/* ------------------------------------------------------------------------- */
void csfg_expr_mark_deleted(struct csfg_expr_pool* pool, int n)
{
    pool->nodes[n].type = CSFG_EXPR_GC;
}

/* ------------------------------------------------------------------------- */
void csfg_expr_mark_deleted_recursive(struct csfg_expr_pool* pool, int n)
{
    int left, right;
    CSFG_DEBUG_ASSERT(n > -1);

    left = pool->nodes[n].child[0];
    right = pool->nodes[n].child[1];

    if (left > -1)
        csfg_expr_mark_deleted_recursive(pool, left);
    if (right > -1)
        csfg_expr_mark_deleted_recursive(pool, right);

    csfg_expr_mark_deleted(pool, n);
}

/* ------------------------------------------------------------------------- */
int csfg_expr_gc(struct csfg_expr_pool* pool, int root)
{
    int n;
    for (n = 0; n < csfg_expr_pool_count(pool); ++n)
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
    CSFG_DEBUG_ASSERT(
        pool->nodes[parent].child[0] == child ||
        pool->nodes[parent].child[1] == child);

    pool->nodes[parent] = pool->nodes[child];

    csfg_expr_mark_deleted(pool, child);
    if (dangling_child > -1)
        csfg_expr_mark_deleted_recursive(pool, dangling_child);
}

/* ------------------------------------------------------------------------- */
void csfg_expr_collapse_sibling_into_parent(struct csfg_expr_pool* pool, int n)
{
    int sibling;
    int parent = csfg_expr_find_parent(pool, n);
    CSFG_DEBUG_ASSERT(parent > -1);
    sibling = pool->nodes[parent].child[0] == n ? pool->nodes[parent].child[1]
                                                : pool->nodes[parent].child[0];
    CSFG_DEBUG_ASSERT(sibling > -1);
    pool->nodes[parent] = pool->nodes[sibling];
    csfg_expr_mark_deleted(pool, sibling);
    csfg_expr_mark_deleted_recursive(pool, n);
}

/* ------------------------------------------------------------------------- */
int csfg_expr_collapse_sibling_into_parent_steal_orphan(
    struct csfg_expr_pool* pool, int n)
{
    int sibling;
    int parent = csfg_expr_find_parent(pool, n);
    CSFG_DEBUG_ASSERT(parent > -1);
    sibling = pool->nodes[parent].child[0] == n ? pool->nodes[parent].child[1]
                                                : pool->nodes[parent].child[0];

    pool->nodes[parent] = pool->nodes[sibling];
    csfg_expr_mark_deleted(pool, sibling);
    return n;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_find_parent(const struct csfg_expr_pool* pool, int n)
{
    int p;
    for (p = 0; p != csfg_expr_pool_count(pool); ++p)
        if (pool->nodes[p].child[0] == n || pool->nodes[p].child[1] == n)
            if (pool->nodes[p].type != CSFG_EXPR_GC)
                return p;
    return -1;
}

/* ------------------------------------------------------------------------- */
int csfg_expr_find_sibling(const struct csfg_expr_pool* pool, int n)
{
    int parent = csfg_expr_find_parent(pool, n);
    if (parent == -1)
        return -1;
    if (pool->nodes[parent].child[0] == n)
        return pool->nodes[parent].child[1];
    return pool->nodes[parent].child[0];
}

/* ------------------------------------------------------------------------- */
int csfg_expr_find_top_of_chain(const struct csfg_expr_pool* pool, int n)
{
    while (1)
    {
        int parent = csfg_expr_find_parent(pool, n);
        if (parent == -1 || pool->nodes[parent].type != pool->nodes[n].type)
            return n;
        n = parent;
    }
}

/* ------------------------------------------------------------------------- */
int csfg_expr_equal(
    const struct csfg_expr_pool* p1,
    int                          root1,
    const struct csfg_expr_pool* p2,
    int                          root2)
{
    int child1, child2;

    if (p1->nodes[root1].type != p2->nodes[root2].type)
        return 0;

    switch (p1->nodes[root1].type)
    {
        case CSFG_EXPR_GC: break;
        case CSFG_EXPR_LIT:
            if (p1->nodes[root1].value.lit != p2->nodes[root2].value.lit)
                return 0;
            break;
        case CSFG_EXPR_VAR: {
            struct strview s1 =
                strlist_view(p1->var_names, p1->nodes[root1].value.var_idx);
            struct strview s2 =
                strlist_view(p2->var_names, p2->nodes[root2].value.var_idx);
            if (strview_eq(s1, s2) == 0)
                return 0;
            break;
        }
        case CSFG_EXPR_INF:
        case CSFG_EXPR_NEG:
        case CSFG_EXPR_OP_ADD:
        case CSFG_EXPR_OP_MUL:
        case CSFG_EXPR_OP_POW: break;
    }

    child1 = p1->nodes[root1].child[0];
    child2 = p2->nodes[root2].child[0];
    if (child1 != -1)
        if (csfg_expr_equal(p1, child1, p2, child2) == 0)
            return 0;

    child1 = p1->nodes[root1].child[1];
    child2 = p2->nodes[root2].child[1];
    if (child1 != -1)
        if (csfg_expr_equal(p1, child1, p2, child2) == 0)
            return 0;

    return 1;
}
