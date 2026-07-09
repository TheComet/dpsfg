#pragma once

#include "csfg/util/vec.h"

struct str;
struct strlist;

struct serializer
{
    int32_t count, capacity;
    uint8_t data[1];
};
static void serializer_init(struct serializer** v)
{
    *v = ((void*)0);
}
void serializer_deinit(struct serializer* v);
int serializer_realloc(struct serializer** v, int32_t new_capacity);
static void serializer_clear(struct serializer* v)
{
    if (v)
        v->count = 0;
}
void serializer_compact(struct serializer** v);
static void serializer_clear_compact(struct serializer** v)
{
    serializer_clear(*v);
    serializer_compact(v);
}
uint8_t* serializer_emplace(struct serializer** v);
uint8_t* serializer_emplace_no_realloc(struct serializer* v);
static int serializer_push(struct serializer** v, uint8_t elem)
{
    uint8_t* ins = serializer_emplace(v);
    if (ins == ((void*)0))
        return -1;
    *ins = elem;
    return 0;
}
static void serializer_push_no_realloc(struct serializer* v, uint8_t elem)
{
    *serializer_emplace_no_realloc(v) = elem;
}
uint8_t* serializer_insert_emplace(struct serializer** v, int32_t i);
static int serializer_insert(struct serializer** v, int32_t i, uint8_t elem)
{
    uint8_t* ins = serializer_insert_emplace(v, i);
    if (ins == ((void*)0))
        return -1;
    *ins = elem;
    return 0;
}
static uint8_t* serializer_pop(struct serializer* v)
{
    ((void)sizeof((v->count > 0) ? 1 : 0), __extension__({
         if (v->count > 0)
             ;
         else
             __assert_fail(
                 "v->count > 0",
                 "/home/thecomet/documents/programming/cpp/dpsfg/csfg/include/csfg/io/serialize.h",
                 8,
                 __extension__ __PRETTY_FUNCTION__);
     }));
    return &v->data[--(v->count)];
}
static uint8_t* serializer_pop_by(struct serializer* v, int32_t count)
{
    return &v->data[v->count -= count];
}
void serializer_erase(struct serializer* v, int32_t i);
static void serializer_swap(struct serializer** a, struct serializer** b)
{
    struct serializer* tmp = *a;
    *a                     = *b;
    *b                     = tmp;
}
static void serializer_swap_values(struct serializer* v, int32_t a, int32_t b)
{
    uint8_t tmp = v->data[a];
    v->data[a]  = v->data[b];
    v->data[b]  = tmp;
}
int serializer_retain(
    struct serializer* v,
    int (*on_element)(uint8_t* elem, void* user),
    void* user);
void serializer_reverse_range(struct serializer* v, int32_t start, int32_t end);
static void serializer_reverse(struct serializer* v)
{
    serializer_reverse_range(v, 0, ((v) ? (v)->count : 0));
}

int serialize_data(struct serializer** ser, const void* data, int len);
int serialize_char(struct serializer** ser, char value);
int serialize_u8(struct serializer** ser, uint8_t value);
int serialize_lu16(struct serializer** ser, uint16_t value);
int serialize_li16(struct serializer** ser, int16_t value);
int serialize_lu32(struct serializer** ser, uint32_t value);
int serialize_li32(struct serializer** ser, int32_t value);
int serialize_lf32(struct serializer** ser, float value);
int serialize_cstr(struct serializer** ser, const char* cstr);
