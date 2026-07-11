#include "csfg/io/deserialize.h"
#include "csfg/io/io.h"
#include "csfg/io/serialize.h"
#include "csfg/symbolic/expr.h"

/* -------------------------------------------------------------------------- */
static int
serialize_expr_pool(struct serializer** ser, const struct csfg_expr_pool* pool)
{
    int n;
    int err = 0;

    err += serialize_li16(ser, pool->count);
    for (n = 0; n != pool->count; ++n)
    {
        err += serialize_u8(ser, pool->nodes[n].type);
        err += serialize_li16(ser, pool->nodes[n].child[0]);
        err += serialize_li16(ser, pool->nodes[n].child[1]);

        switch ((enum csfg_expr_type)pool->nodes[n].type)
        {
            case CSFG_EXPR_GC: CSFG_DEBUG_ASSERT(0); return -1;
            case CSFG_EXPR_LIT:
                err += serialize_lf32(ser, pool->nodes[n].value.lit);
                break;
            case CSFG_EXPR_VAR:
                err += serialize_cstr(
                    ser,
                    strlist_cstr(
                        pool->var_names, pool->nodes[n].value.var_idx));
                break;
            case CSFG_EXPR_INF:
            case CSFG_EXPR_NEG:
            case CSFG_EXPR_ADD:
            case CSFG_EXPR_MUL:
            case CSFG_EXPR_POW: break;
        }
    }

    return err;
}

/* -------------------------------------------------------------------------- */
static int save_expr_recurse(
    struct serializer** ser, const struct csfg_expr_pool* pool, int expr)
{
    enum csfg_expr_type type;
    int left, right, var_idx, err = 0;
    uint8_t active_children;

    if (expr < 0)
        return 0;

    type  = (enum csfg_expr_type)pool->nodes[expr].type;
    left  = pool->nodes[expr].child[0];
    right = pool->nodes[expr].child[1];

    active_children = (left > -1 ? 0x01 : 0x00) | (right > -1 ? 0x02 : 0x00);

    err += serialize_u8(ser, type);
    err += serialize_u8(ser, active_children);

    switch (type)
    {
        case CSFG_EXPR_GC: CSFG_DEBUG_ASSERT(0); return -1;
        case CSFG_EXPR_LIT:
            err += serialize_lf32(ser, pool->nodes[expr].value.lit);
            break;
        case CSFG_EXPR_VAR:
            var_idx = pool->nodes[expr].value.var_idx;
            err += serialize_cstr(ser, strlist_cstr(pool->var_names, var_idx));
            break;
        case CSFG_EXPR_INF:
        case CSFG_EXPR_NEG:
        case CSFG_EXPR_ADD:
        case CSFG_EXPR_MUL:
        case CSFG_EXPR_POW: break;
    }

    if (left > -1)
        err += save_expr_recurse(ser, pool, left);
    if (right > -1)
        err += save_expr_recurse(ser, pool, right);

    return err;
}
int csfg_io_expr_save(
    struct serializer** ser, const struct csfg_expr_pool* pool, int expr)
{
    const uint8_t version = 0;
    int err               = 0;

    err += serialize_u8(ser, version);
    err += serialize_u8(ser, expr > -1 ? 1 : 0);
    err += save_expr_recurse(ser, pool, expr);

    return err;
}

/* -------------------------------------------------------------------------- */
static int load_0_recurse(
    struct deserializer* des, struct csfg_expr_pool** pool, int* expr)
{
    enum csfg_expr_type type = deserialize_u8(des);
    uint8_t active_children  = deserialize_u8(des);

    switch (type)
    {
        case CSFG_EXPR_GC: return log_err("GC node read!\n");
        case CSFG_EXPR_LIT:
            *expr = csfg_expr_lit(pool, deserialize_lf32(des));
            if (*expr < 0)
                return -1;
            if (active_children)
                return log_err("LIT cannot have left/right children!\n");
            return 0;
        case CSFG_EXPR_VAR:
            *expr = csfg_expr_var(pool, cstr_view(deserialize_cstr(des)));
            if (*expr < 0)
                return -1;
            if (active_children)
                return log_err("VAR cannot have left/right children!\n");
            return 0;
        case CSFG_EXPR_INF:
            *expr = csfg_expr_inf(pool);
            if (*expr < 0)
                return -1;
            if (active_children)
                return log_err("INF cannot have left/right children!\n");
            return 0;
        case CSFG_EXPR_NEG: {
            int left;
            if (active_children & 0x02)
                return log_err("NEG cannot have a right-side child!\n");
            if (load_0_recurse(des, pool, &left) != 0)
                return -1;
            *expr = csfg_expr_neg(pool, left);
            if (*expr < 0)
                return -1;
            return 0;
        }
        case CSFG_EXPR_ADD:
        case CSFG_EXPR_MUL:
        case CSFG_EXPR_POW: {
            int left, right;
            if (active_children != 0x03)
                return log_err("binop must have both left/right children!\n");
            if (load_0_recurse(des, pool, &left) != 0)
                return -1;
            if (load_0_recurse(des, pool, &right) != 0)
                return -1;
            *expr = csfg_expr_binop(pool, type, left, right);
            if (*expr < 0)
                return -1;
            return 0;
        }
    }

    return log_err("Invalid node type read: %d\n", type);
}
static int
load_0(struct deserializer* des, struct csfg_expr_pool** pool, int* expr)
{
    int have_expr = deserialize_u8(des);
    if (have_expr)
    {
        if (load_0_recurse(des, pool, expr) != 0)
            return -1;
        return csfg_expr_integrity_check(*pool, *expr);
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
int csfg_io_expr_load(
    struct deserializer* des, struct csfg_expr_pool** pool, int* expr)
{
    uint8_t version = deserialize_u8(des);
    if (deserializer_err(des))
        return log_err("Failed to read version: EOF\n");

    switch (version)
    {
        case 0x00: return load_0(des, pool, expr);
    }

    return log_err("Unsupported format version: %d\n", version);
}
