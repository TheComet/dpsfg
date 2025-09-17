#include "csfg/util/str.h"
#include "csfg/util/vec.h"
#include <stdio.h>

#if defined(_WIN32)
#    define SEP     '\\'
#    define BAD_SEP '/'
#else
#    define SEP     '/'
#    define BAD_SEP '\\'
#endif

VEC_DECLARE(str_impl, char, 16)
VEC_DEFINE(str_impl, char, 16)

/* ------------------------------------------------------------------------- */
static int is_sep(char c)
{
    return c == '/' || c == '\\';
}

/* ------------------------------------------------------------------------- */
int str_init(struct str** str)
{
    str_impl_init((struct str_impl**)str);
    return 0;
}

/* ------------------------------------------------------------------------- */
void str_deinit(struct str* str)
{
    str_impl_deinit((struct str_impl*)str);
}

/* ------------------------------------------------------------------------- */
int str_ensure_capacity(struct str** str, int capacity)
{
    struct str_impl* impl = (struct str_impl*)*str;
    if (impl == NULL)
    {
        if (str_impl_realloc((struct str_impl**)str, capacity + 1) != 0)
            return -1;
        impl = (struct str_impl*)*str;
        impl->data[0] = '\0';
        impl->count = 1;
        return 0;
    }

    if (impl->capacity < capacity + 1)
    {
        if (str_impl_realloc((struct str_impl**)str, capacity + 1) != 0)
            return -1;
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
int str_capacity(const struct str* str)
{
    return vec_capacity((const struct str_impl*)str);
}

/* ------------------------------------------------------------------------- */
int str_len(const struct str* str)
{
    if (str != NULL)
    {
        assert(((const struct str_impl*)str)->count > 0);
    }
    return str ? ((const struct str_impl*)str)->count - 1 : 0;
}

/* ------------------------------------------------------------------------- */
char* str_data(struct str* str)
{
    return str ? ((struct str_impl*)str)->data : NULL;
}

/* ------------------------------------------------------------------------- */
int str_append_char(struct str** str, char c)
{
    struct str_impl* impl;
    int              len = str_len(*str);

    if (str_ensure_capacity(str, len + 1) != 0)
        return -1;
    impl = (struct str_impl*)*str;

    impl->data[len] = c;
    impl->data[len + 1] = '\0';
    impl->count = len + 2;

    return 0;
}

/* ------------------------------------------------------------------------- */
int str_append_cstr(struct str** str, const char* cstr)
{
    struct str_impl* impl;
    int              len = str_len(*str);
    int              clen = (int)strlen(cstr);

    if (str_ensure_capacity(str, len + clen) != 0)
        return -1;
    impl = (struct str_impl*)*str;

    memcpy(impl->data + len, cstr, clen);
    impl->data[len + clen] = '\0';
    impl->count = len + clen + 1;

    return 0;
}

/* ------------------------------------------------------------------------- */
int str_append_int(struct str** str, int value)
{

    char buf[12];
    sprintf(buf, "%d", value);
    return str_append_cstr(str, buf);
}

/* ------------------------------------------------------------------------- */
void str_pop_char(struct str* str)
{
    struct str_impl* impl = (struct str_impl*)str;
    if (str_len(str) == 0)
        return;
    impl->data[--impl->count] = '\0';
}

/* ------------------------------------------------------------------------- */
void str_set_char(struct str* str, int index, char c)
{
    struct str_impl* impl = (struct str_impl*)str;
    assert(index >= 0 && index < vec_capacity(impl) - 1);
    impl->data[index] = c;
}

/* ------------------------------------------------------------------------- */
void str_clear(struct str* str)
{
    struct str_impl* impl = (struct str_impl*)str;
    if (impl == NULL)
        return;
    impl->data[0] = '\0';
    impl->count = 1;
}

/* ------------------------------------------------------------------------- */
void str_set_len(struct str* str, int new_len)
{
    struct str_impl* impl = (struct str_impl*)str;
    assert(new_len >= 0 && new_len <= vec_capacity(impl) - 1);

    impl->data[new_len] = '\0';
    impl->count = new_len + 1;
}

/* ------------------------------------------------------------------------- */
int str_set(struct str** str, const char* data, int len)
{
    struct str_impl* impl;

    if (str_ensure_capacity(str, len) != 0)
        return -1;
    impl = (struct str_impl*)*str;

    memcpy(impl->data, data, len);
    impl->data[len] = '\0';
    impl->count = len + 1;

    return 0;
}

/* ------------------------------------------------------------------------- */
int str_set_str(struct str** str, const struct str* other)
{
    const struct str_impl* impl = (const struct str_impl*)other;
    return str_set(str, impl->data, impl->count - 1);
}

/* ------------------------------------------------------------------------- */
int str_set_view(struct str** str, struct strview view)
{
    return str_set(str, view.data + view.off, view.len);
}

/* ------------------------------------------------------------------------- */
int str_set_utf32(struct str** str, const uint32_t* utf32, int len)
{
    str_clear(*str);
    if (utf32 == NULL || len == 0)
        return 0;

    if (str_ensure_capacity(str, len * (int)sizeof(uint32_t)) != 0)
        return -1;

    while (len--)
    {
        uint32_t cp = *utf32++;
        if (cp <= 0x7F)
        {
            str_append_char(str, (char)(cp & 0x7F));
        }
        else if (cp <= 0x7FF)
        {
            str_append_char(str, (char)(0xC0 | ((cp >> 6) & 0x1F)));
            str_append_char(str, (char)(0x80 | (cp & 0x3F)));
        }
        else if (cp <= 0xFFFF)
        {
            str_append_char(str, (char)(0xE0 | ((cp >> 12) & 0x0F)));
            str_append_char(str, (char)(0x80 | ((cp >> 6) & 0x3F)));
            str_append_char(str, (char)(0x80 | (cp & 0x3F)));
        }
        else if (cp <= 0x10FFFF)
        {
            str_append_char(str, (char)(0xF0 | ((cp >> 18) & 0x07)));
            str_append_char(str, (char)(0x80 | ((cp >> 12) & 0x3F)));
            str_append_char(str, (char)(0x80 | ((cp >> 6) & 0x3F)));
            str_append_char(str, (char)(0x80 | (cp & 0x3F)));
        }
        else
        {
            return log_err("Invalid UTF-32 code point: %x\n", cp);
        }
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
int str_set_cstr(struct str** str, const char* cstr)
{
    return str_set(str, cstr, (int)strlen(cstr));
}

/* ------------------------------------------------------------------------- */
const char* str_cstr(const struct str* str)
{
    return str ? ((const struct str_impl*)str)->data : "";
}

/* ------------------------------------------------------------------------- */
int str_eq_cstr(const struct str* str, const char* cstr)
{
    const struct str_impl* impl = (const struct str_impl*)str;
    return strcmp(impl->data, cstr) == 0;
}

/* ------------------------------------------------------------------------- */
int str_set_path_cstr(struct str** str, const char* path)
{
    int              i;
    struct str_impl* impl;
    int              len = (int)strlen(path);

    if (str_ensure_capacity(str, len) != 0)
        return -1;
    impl = (struct str_impl*)*str;

    memcpy(impl->data, path, len);
    impl->data[len] = '\0';
    impl->count = len + 1;

    for (i = 0; i != len; ++i)
        if (impl->data[i] == BAD_SEP)
            impl->data[i] = SEP;

    return 0;
}

/* ------------------------------------------------------------------------- */
int str_join_path(struct str** str, struct strview path)
{
    struct str_impl* impl;
    int              len = str_len(*str);
    int              sep_len = 1;

    if (str_ensure_capacity(str, len + path.len + sep_len) != 0)
        return -1;
    impl = (struct str_impl*)*str;

    if (!is_sep(impl->data[len]))
    {
        impl->data[len] = SEP;
        len++;
    }

    memcpy(impl->data + len, path.data + path.off, path.len);
    len += path.len;

    impl->data[len] = '\0';
    impl->count = len + 1;

    return 0;
}

/* ------------------------------------------------------------------------- */
int str_join_path_cstr(struct str** str, const char* path)
{
    return str_join_path(str, strview(path, 0, (int)strlen(path)));
}

/* ------------------------------------------------------------------------- */
int str_join_path_prepend(struct str** str, struct strview path)
{
    struct str_impl* impl;
    int              len = str_len(*str);
    int              sep_len = 1;

    if (is_sep(path.data[path.off + path.len - 1]))
        sep_len = 0;

    if (str_ensure_capacity(str, len + path.len + sep_len) != 0)
        return -1;
    impl = (struct str_impl*)*str;

    /* Move with NULL terminator */
    memmove(impl->data + path.len + sep_len, impl->data, len + 1);
    memcpy(impl->data, path.data, path.len);
    impl->data[path.len] = SEP;
    impl->count = len + sep_len + path.len + 1;

    return 0;
}

/* ------------------------------------------------------------------------- */
int str_join_path_prepend_cstr(struct str** str, const char* path)
{
    return str_join_path_prepend(str, strview(path, 0, (int)strlen(path)));
}

/* ------------------------------------------------------------------------- */
void str_dirname(struct str* str)
{
    struct str_impl* impl = (struct str_impl*)str;

    /* Remove trailing slashes (if not root) */
    while (impl->count > 2 && is_sep(impl->data[impl->count - 2]))
        impl->count--;
    /* Remove file name */
    while (impl->count > 2 && !is_sep(impl->data[impl->count - 2]))
        impl->count--;
    /* Remove joining slash if not root directory */
    while (impl->count > 2 && is_sep(impl->data[impl->count - 2]))
        impl->count--;
    /* Special case on linux -- root directory */
    if (impl->count == 2 && !is_sep(impl->data[impl->count - 2]))
        impl->count--;

    assert(impl->count > 1);
    impl->data[impl->count - 1] = '\0';
}

/* ------------------------------------------------------------------------- */
const char* cstr_ext(const char* path)
{
    const char* ext = strrchr(path, '.');
    if (ext == NULL || ext == path || is_sep(*(ext - 1)))
        return "";
    return ext + 1;
}

/* ------------------------------------------------------------------------- */
int cstr_ends_with(const char* cstr, const char* suffix)
{
    int cstr_len = (int)strlen(cstr);
    int suffix_len = (int)strlen(suffix);

    if (cstr_len < suffix_len)
        return 0;

    return strcmp(cstr + cstr_len - suffix_len, suffix) == 0;
}
