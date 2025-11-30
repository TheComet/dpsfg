#include "csfg/io/deserialize.h"
#include <string.h>

/* -------------------------------------------------------------------------- */
void deserialize_data(struct deserializer* des, void* dst, int len)
{
    if (des->size - des->read_offset < len)
    {
        des->err = 1;
        return;
    }
    memcpy(dst, des->data + des->read_offset, len);
    des->read_offset += len;
}

/* -------------------------------------------------------------------------- */
char deserialize_char(struct deserializer* des)
{
    if (des->size - des->read_offset < 1)
        return des->err = 1, 0;
    return (char)des->data[des->read_offset++];
}

/* -------------------------------------------------------------------------- */
uint8_t deserialize_u8(struct deserializer* des)
{
    if (des->size - des->read_offset < 1)
        return des->err = 1, 0;
    return (uint8_t)des->data[des->read_offset++];
}

/* -------------------------------------------------------------------------- */
uint16_t deserialize_lu16(struct deserializer* des)
{
    uint16_t value;
    if (des->size - des->read_offset < 2)
        return des->err = 1, 0;
    value = ((des->data[des->read_offset + 0] & 0xFF) << 0) |
            ((des->data[des->read_offset + 1] & 0xFF) << 8);
    des->read_offset += 2;
    return value;
}

/* -------------------------------------------------------------------------- */
int16_t deserialize_li16(struct deserializer* des)
{
    return (int16_t)deserialize_lu16(des);
}

/* -------------------------------------------------------------------------- */
uint32_t deserialize_lu32(struct deserializer* des)
{
    uint32_t value;
    if (des->size - des->read_offset < 4)
        return des->err = 1, 0;
    value = ((des->data[des->read_offset + 0] & 0xFF) << 0) |
            ((des->data[des->read_offset + 1] & 0xFF) << 8) |
            ((des->data[des->read_offset + 2] & 0xFF) << 16) |
            ((des->data[des->read_offset + 3] & 0xFF) << 24);
    des->read_offset += 4;
    return value;
}

/* -------------------------------------------------------------------------- */
int32_t deserialize_li32(struct deserializer* des)
{
    return (int32_t)deserialize_lu32(des);
}

/* -------------------------------------------------------------------------- */
float deserialize_lf32(struct deserializer* des)
{
    float    value;
    uint32_t u32 = deserialize_lu32(des);
    memcpy(&value, &u32, 4);
    return value;
}

/* -------------------------------------------------------------------------- */
const char* deserialize_cstr(struct deserializer* des)
{
    const char* cstr = des->data + des->read_offset;
    while (des->data[des->read_offset++] != '\0')
        if (des->read_offset >= des->size)
            return "";
    return cstr;
}

/* -------------------------------------------------------------------------- */
int deserializer_err(const struct deserializer* des)
{
    return des->err;
}
