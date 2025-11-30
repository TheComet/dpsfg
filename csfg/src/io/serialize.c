#include "csfg/io/serialize.h"
#include "csfg/util/str.h"
#include "csfg/util/strlist.h"

VEC_DEFINE(serializer, uint8_t, 32)

/* -------------------------------------------------------------------------- */
int serialize_data(struct serializer** ser, const void* data, int len)
{
    const uint8_t* p = data;
    if (serializer_realloc(ser, vec_count(*ser) + len) != 0)
        return -1;
    while (len--)
        serializer_push(ser, *p++);
    return 0;
}

/* -------------------------------------------------------------------------- */
int serialize_char(struct serializer** ser, char value)
{
    return serializer_push(ser, (uint8_t)value);
}

/* -------------------------------------------------------------------------- */
int serialize_u8(struct serializer** ser, uint8_t value)
{
    return serializer_push(ser, value);
}

/* -------------------------------------------------------------------------- */
int serialize_lu16(struct serializer** ser, uint16_t value)
{
    uint8_t data[2];
    data[0] = (value >> 0) & 0xFF;
    data[1] = (value >> 8) & 0xFF;
    return serialize_data(ser, data, 2);
}

/* -------------------------------------------------------------------------- */
int serialize_li16(struct serializer** ser, int16_t value)
{
    return serialize_lu16(ser, (uint16_t)value);
}

/* -------------------------------------------------------------------------- */
int serialize_lu32(struct serializer** ser, uint32_t value)
{
    uint8_t data[4];
    data[0] = (value >> 0) & 0xFF;
    data[1] = (value >> 8) & 0xFF;
    data[2] = (value >> 16) & 0xFF;
    data[3] = (value >> 24) & 0xFF;
    return serialize_data(ser, data, 4);
}

/* -------------------------------------------------------------------------- */
int serialize_li32(struct serializer** ser, int32_t value)
{
    return serialize_lu32(ser, (uint32_t)value);
}

/* -------------------------------------------------------------------------- */
int serialize_lf32(struct serializer** ser, float value)
{
    uint32_t u32;
    memcpy(&u32, &value, 4);
    return serialize_lu32(ser, u32);
}

/* -------------------------------------------------------------------------- */
int serialize_cstr(struct serializer** ser, const char* cstr)
{
    return serialize_data(ser, cstr, strlen(cstr) + 1);
}
