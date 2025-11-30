#pragma once

#include <stdint.h>

struct strlist;

struct deserializer
{
    const char* data;
    int         size;
    int         read_offset;
    unsigned    err : 1;
};

static struct deserializer deserializer(const void* data, int size)
{
    struct deserializer des;
    des.data = (const char*)data;
    des.size = size;
    des.read_offset = 0;
    des.err = 0;
    return des;
}

void        deserialize_data(struct deserializer* des, void* dst, int len);
char        deserialize_char(struct deserializer* des);
uint8_t     deserialize_u8(struct deserializer* des);
uint16_t    deserialize_lu16(struct deserializer* des);
int16_t     deserialize_li16(struct deserializer* des);
uint32_t    deserialize_lu32(struct deserializer* des);
int32_t     deserialize_li32(struct deserializer* des);
float       deserialize_lf32(struct deserializer* des);
const char* deserialize_cstr(struct deserializer* des);

int deserializer_err(const struct deserializer* des);
