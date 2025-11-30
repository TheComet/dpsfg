#pragma once

#include "csfg/util/vec.h"

struct str;
struct strlist;

VEC_DECLARE(serializer, uint8_t, 32)

int serialize_data(struct serializer** ser, const void* data, int len);
int serialize_char(struct serializer** ser, char value);
int serialize_u8(struct serializer** ser, uint8_t value);
int serialize_lu16(struct serializer** ser, uint16_t value);
int serialize_li16(struct serializer** ser, int16_t value);
int serialize_lu32(struct serializer** ser, uint32_t value);
int serialize_li32(struct serializer** ser, int32_t value);
int serialize_lf32(struct serializer** ser, float value);
int serialize_cstr(struct serializer** ser, const char* cstr);
