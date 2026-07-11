#include "csfg/io/deserialize.h"
#include "csfg/io/io.h"
#include "csfg/io/serialize.h"
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/var_table.h"

/* -------------------------------------------------------------------------- */
int csfg_io_var_table_save(
    struct serializer** ser, const struct csfg_var_table* vt)
{
    int16_t slot;
    struct str* name;
    struct csfg_var_table_entry* entry;

    const uint8_t version = 0;
    int err               = 0;

    err += serialize_u8(ser, version);

    err += serialize_li16(ser, hmap_count(vt->map));
    hmap_for_each (vt->map, slot, name, entry)
    {
        err += serialize_cstr(ser, str_cstr(name));
        err += csfg_io_expr_save(ser, entry->pool, entry->expr);
    }

    return err;
}

/* -------------------------------------------------------------------------- */
static int load_0(struct deserializer* des, struct csfg_var_table* vt)
{
    int16_t entry_count = deserialize_li16(des);
    if (entry_count < 0 || entry_count > 1000)
        return log_err(
            "Variable table entry count is implausible: %d\n", entry_count);

    while (entry_count-- > 0)
    {
        struct csfg_expr_pool* pool;
        int expr;
        const char* name;

        csfg_expr_pool_init(&pool);

        name = deserialize_cstr(des);
        if (csfg_io_expr_load(des, &pool, &expr) != 0)
            goto fail;

        if (csfg_var_table_set_expr(vt, cstr_view(name), pool, expr) != 0)
            goto fail;

        continue;

    fail:
        csfg_expr_pool_deinit(pool);
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
int csfg_io_var_table_load(struct deserializer* des, struct csfg_var_table* vt)
{
    uint8_t version = deserialize_u8(des);
    if (deserializer_err(des))
        return log_err("Failed to read version: EOF\n");

    csfg_var_table_clear(vt);
    switch (version)
    {
        case 0x00: return load_0(des, vt);
    }

    return log_err("Unsupported format version: %d\n", version);
}
