#include "csfg/symbolic/expr.h"
#include "csfg/util/vec.h"
#include "mathomatic/externs.h"
#include "mathomatic/simplify.h"

VEC_DECLARE(token_vec, token_type, 32)
int token_vec_realloc(struct token_vec** v, int32_t elems)
{
    int header, data;
    struct token_vec* new_v;
    if (elems == 0)
    {
        if (*v)
        {
            mem_free(*v);
            *v = ((void*)0);
        }
        return 0;
    }
    if (elems >= (1 << (32 - 2)))
        do
        {
        } while (0);
    header = __builtin_offsetof(struct token_vec, data);
    data   = sizeof(token_type) * elems;
    new_v  = (struct token_vec*)mem_realloc(*v, header + data);
    if (new_v == ((void*)0))
        return log_oom(header + data, "vec_realloc()");
    if (*v == ((void*)0))
        new_v->count = 0;
    if (new_v->count > elems)
        new_v->count = elems;
    new_v->capacity = elems;
    *v              = new_v;
    return 0;
}
void token_vec_deinit(struct token_vec* v)
{
    if (v)
        mem_free(v);
}
void token_vec_compact(struct token_vec** v)
{
    (void)token_vec_realloc(v, ((*v) ? (*v)->count : 0));
}
token_type* token_vec_emplace(struct token_vec** v)
{
    if (*v == ((void*)0) || (*v)->count == (*v)->capacity)
    {
        int old_cap = *v ? (*v)->capacity : 0;
        int new_cap = old_cap ? old_cap * 2 : 32;
        if (token_vec_realloc(v, new_cap) != 0)
            return ((void*)0);
        if (old_cap == 0)
            (*v)->count = 0;
        (*v)->capacity = new_cap;
    }
    return &(*v)->data[(*v)->count++];
}
token_type* token_vec_emplace_no_realloc(struct token_vec* v)
{
    ((void)sizeof((v != ((void*)0) && v->count < v->capacity) ? 1 : 0),
     __extension__({
         if (v != ((void*)0) && v->count < v->capacity)
             ;
         else
             __assert_fail(
                 "v != NULL && v->count < v->capacity",
                 "/home/thecomet/documents/programming/cpp/dpsfg/csfg/src/symbolic/expr_simplify.c",
                 7,
                 __extension__ __PRETTY_FUNCTION__);
     }));
    return &v->data[v->count++];
}
token_type* token_vec_insert_emplace(struct token_vec** v, int32_t i)
{
    ((void)sizeof((i >= 0 && i <= (*v ? (*v)->count : 0)) ? 1 : 0),
     __extension__({
         if (i >= 0 && i <= (*v ? (*v)->count : 0))
             ;
         else
             __assert_fail(
                 "i >= 0 && i <= (*v ? (*v)->count : 0)",
                 "/home/thecomet/documents/programming/cpp/dpsfg/csfg/src/symbolic/expr_simplify.c",
                 7,
                 __extension__ __PRETTY_FUNCTION__);
     }));
    if (token_vec_emplace(v) == ((void*)0))
        return ((void*)0);
    memmove(
        (*v)->data + i + 1,
        (*v)->data + i,
        ((*v)->count - i - 1) * sizeof(token_type));
    return &(*(v))->data[i];
}
void token_vec_erase(struct token_vec* v, int32_t i)
{
    --(v->count);
    memmove(v->data + i, v->data + i + 1, (v->count - i) * sizeof(token_type));
}
int token_vec_retain(
    struct token_vec* v,
    int (*on_element)(token_type* elem, void* user),
    void* user)
{
    int32_t i;
    if (v == ((void*)0))
        return 0;
    for (i = 0; i != v->count; ++i)
    {
        int result = on_element(&v->data[i], user);
        if (result < 0)
            return result;
        if (result != 0)
        {
            token_vec_erase(v, i);
            --i;
        }
    }
    return 0;
}
void token_vec_reverse_range(struct token_vec* v, int32_t start, int32_t end)
{
    ((void)sizeof(
         (start >= 0 && start < end && end <= ((v) ? (v)->count : 0)) ? 1 : 0),
     __extension__({
         if (start >= 0 && start < end && end <= ((v) ? (v)->count : 0))
             ;
         else
             __assert_fail(
                 "start >= 0 && start < end && end <= vec_count(v)",
                 "/home/thecomet/documents/programming/cpp/dpsfg/csfg/src/symbolic/expr_simplify.c",
                 7,
                 __extension__ __PRETTY_FUNCTION__);
     }));
    while (start < --end)
        token_vec_swap_values(v, start++, end);
}

/* -------------------------------------------------------------------------- */
static int expr_to_mathomatic_equation(
    struct token_vec** eq,
    const struct csfg_expr_pool* pool,
    int expr,
    int depth)
{
    int var_idx;
    struct token_type* tok;
    enum csfg_expr_type type;

    type = pool->nodes[expr].type;
    switch (type)
    {
        case CSFG_EXPR_GC: CSFG_DEBUG_ASSERT(0); break;
        case CSFG_EXPR_LIT:
            tok = token_vec_emplace(eq);
            if (tok == NULL)
                return -1;
            tok->kind           = CONSTANT;
            tok->level          = depth;
            tok->token.constant = pool->nodes[expr].value.lit;
            break;
        case CSFG_EXPR_VAR:
            /* Mathomatic stores variable names in a global array called
             * "var_names". Each token indexes into this array. Normally,
             * mathomatic would use malloc() to create an entry, but it looks
             * like none of the important operations cause any reallocations,
             * so we can just copy over our pointers instead of having to copy
             * over our strings.
             */
            var_idx = pool->nodes[expr].value.var_idx;
            if (var_idx >= MAX_VAR_NAMES)
                return log_err(
                    "Variable at idx=%d exceeds mathomatic's maximum variable storage size of %d\n",
                    var_idx,
                    MAX_VAR_NAMES);
            var_names[var_idx] = strlist_data(pool->var_names, var_idx);

            tok = token_vec_emplace(eq);
            if (tok == NULL)
                return -1;
            tok->kind           = VARIABLE;
            tok->level          = depth;
            tok->token.variable = var_idx + VAR_OFFSET;
            break;
        case CSFG_EXPR_INF:
            /* We cannot simplify the equation meaningfully if it contains
             * infinities. Make sure to run csfg_expr_apply_limits() first! */
            log_dbg("csfg_expr_simplify() failed: Contains infinities\n");
            return -1;
        case CSFG_EXPR_NEG:
            tok = token_vec_emplace(eq);
            if (tok == NULL)
                return -1;
            tok->kind           = CONSTANT;
            tok->level          = depth;
            tok->token.constant = -1.0;

            tok = token_vec_emplace(eq);
            if (tok == NULL)
                return -1;
            tok->kind          = OPERATOR;
            tok->level         = depth;
            tok->token.operatr = TIMES;

            return expr_to_mathomatic_equation(
                eq, pool, pool->nodes[expr].child[0], depth + 1);
        case CSFG_EXPR_ADD:
        case CSFG_EXPR_MUL:
        case CSFG_EXPR_POW:
            if (expr_to_mathomatic_equation(
                    eq, pool, pool->nodes[expr].child[0], depth + 1) != 0)
                return -1;

            tok = token_vec_emplace(eq);
            if (tok == NULL)
                return -1;
            tok->kind          = OPERATOR;
            tok->level         = depth;
            tok->token.operatr = type == CSFG_EXPR_ADD ? PLUS
                               : type == CSFG_EXPR_MUL ? TIMES
                                                       : POWER;

            if (expr_to_mathomatic_equation(
                    eq, pool, pool->nodes[expr].child[1], depth + 1) != 0)
                return -1;
            break;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
static int mathomatic_equation_to_expr_recurse(
    struct csfg_expr_pool** pool,
    const struct token_vec* eq,
    int* idx,
    int level,
    int div_flip_flop)
{
    int (*make_op)(struct csfg_expr_pool**, int, int);
    int expr = -1;
    do
    {
        const struct token_type* tok = vec_get(eq, *idx);
        switch (tok->kind)
        {
            case CONSTANT:
                CSFG_DEBUG_ASSERT(expr == -1);
                if (tok->level > level)
                    expr = mathomatic_equation_to_expr_recurse(
                        pool, eq, idx, level + 1, div_flip_flop);
                else
                {
                    ++*idx;
                    expr = csfg_expr_lit(pool, tok->token.constant);
                }
                break;
            case VARIABLE:
                CSFG_DEBUG_ASSERT(expr == -1);
                if (tok->level > level)
                    expr = mathomatic_equation_to_expr_recurse(
                        pool, eq, idx, level + 1, div_flip_flop);
                else
                {
                    expr = csfg_expr_new(pool, CSFG_EXPR_VAR, -1, -1);
                    if (expr < 0)
                        return -1;
                    (*pool)->nodes[expr].value.var_idx =
                        tok->token.variable - VAR_OFFSET;
                    ++*idx;
                }
                break;
            case OPERATOR:
                if (level > tok->level)
                    return expr;
                switch (tok->token.operatr)
                {
                    case PLUS : make_op = csfg_expr_add; break;
                    case TIMES: make_op = csfg_expr_mul; break;
                    case POWER: make_op = csfg_expr_pow; break;
                    case MINUS: make_op = csfg_expr_sub; break;
                    case DIVIDE:
                        make_op = div_flip_flop ? csfg_expr_mul : csfg_expr_div;
                        div_flip_flop = 1 - div_flip_flop;
                        break;
                    default: CSFG_DEBUG_ASSERT(0); return -1;
                }
                ++*idx;
                expr = make_op(
                    pool,
                    expr,
                    mathomatic_equation_to_expr_recurse(
                        pool, eq, idx, level, div_flip_flop));
                break;
        }
    } while (*idx != vec_count(eq));

    return expr;
}
static int mathomatic_equation_to_expr(
    struct csfg_expr_pool** pool, const struct token_vec* eq)
{
    int idx = 0;
    return mathomatic_equation_to_expr_recurse(pool, eq, &idx, 1, 0);
}

/* -------------------------------------------------------------------------- */
static void unlink_variable_names_from_mathomatic(
    const struct csfg_expr_pool* pool, int expr)
{
    int var_idx;
    enum csfg_expr_type type = pool->nodes[expr].type;
    switch (type)
    {
        case CSFG_EXPR_GC : CSFG_DEBUG_ASSERT(0); break;
        case CSFG_EXPR_LIT: break;
        case CSFG_EXPR_VAR:
            var_idx            = pool->nodes[expr].value.var_idx;
            var_names[var_idx] = NULL;
            break;
        case CSFG_EXPR_INF: break;
        case CSFG_EXPR_NEG:
            unlink_variable_names_from_mathomatic(
                pool, pool->nodes[expr].child[0]);
            break;
        case CSFG_EXPR_ADD:
        case CSFG_EXPR_MUL:
        case CSFG_EXPR_POW:
            unlink_variable_names_from_mathomatic(
                pool, pool->nodes[expr].child[0]);
            unlink_variable_names_from_mathomatic(
                pool, pool->nodes[expr].child[1]);
            break;
    }
}

/* -------------------------------------------------------------------------- */
int csfg_expr_simplify(struct csfg_expr_pool** pool, int expr)
{
    struct token_vec* eq;
    int new_expr;

    int quick_flag  = 0;
    int repeat_flag = 1;

    token_vec_init(&eq);
    token_vec_realloc(&eq, n_tokens);

    if (expr_to_mathomatic_equation(&eq, *pool, expr, 1) != 0)
        goto convert_to_mathomatic_failed;
    simpa_repeat_side(vec_data(eq), &eq->count, quick_flag, repeat_flag);
    new_expr = mathomatic_equation_to_expr(pool, eq);

    unlink_variable_names_from_mathomatic(*pool, expr);
    token_vec_deinit(eq);

    return new_expr;

convert_to_mathomatic_failed:
    unlink_variable_names_from_mathomatic(*pool, expr);
    token_vec_deinit(eq);
    return -1;
}
