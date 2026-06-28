#define _POSIX_C_SOURCE 200809L
#define MAX_COL_WIDTH   32

#include "backtrace.h"
#include "csfg/util/backtrace.h"
#include "csfg/util/vec.h"
#include <stdio.h>

#define MIN(a, b)      ((a) < (b) ? (a) : (b))
#define MAX(a, b)      ((a) > (b) ? (a) : (b))
#define CLAMP(v, a, b) MAX(MIN(v, b), a)

static struct backtrace_state* bt_state;

VEC_DECLARE(backtrace_vec, uintptr_t, 16)
VEC_DEFINE(backtrace_vec, uintptr_t, 16)

struct bt_columns
{
    int file;
    int func;
};

/* -------------------------------------------------------------------------- */
static void bt_error(void* data, const char* msg, int errnum)
{
    log_err("%s: %d\n", msg, errnum);
    (void)data;
}

/* -------------------------------------------------------------------------- */
static int bt_simple(void* data, uintptr_t pc)
{
    struct backtrace_vec** bt = data;
    if (vec_count(*bt) >= 63)
        return -1;
    return backtrace_vec_push(bt, pc);
}

/* -------------------------------------------------------------------------- */
static int bt_full_calc_columns(
    void* data,
    uintptr_t pc,
    const char* filename,
    int lineno,
    const char* function)
{
    struct bt_columns* col = data;

    int file  = snprintf(NULL, 0, "%s:%d", filename, lineno);
    int func  = snprintf(NULL, 0, "%s", function);
    col->file = CLAMP(file, col->file, MAX_COL_WIDTH);
    col->func = CLAMP(func, col->func, MAX_COL_WIDTH);

    (void)pc;
    return 0;
}

/* -------------------------------------------------------------------------- */
static int bt_full_print(
    void* data,
    uintptr_t pc,
    const char* filename,
    int lineno,
    const char* function)
{
    int off;
    char buf[MAX_COL_WIDTH + 1];
    struct bt_columns* col = data;

    int file = snprintf(NULL, 0, "%s:%d", filename, lineno);
    int func = snprintf(NULL, 0, "%s", function);

    if (filename == NULL || function == NULL)
        return -1;

    off = file > MAX_COL_WIDTH ? file - MAX_COL_WIDTH : 0;
    snprintf(buf, sizeof(buf), "%s:%d", filename + off, lineno);
    log_raw("  %-*s", col->file, buf);

    off = func > MAX_COL_WIDTH ? func - MAX_COL_WIDTH : 0;
    snprintf(buf, sizeof(buf), "%s()", function + off);
    log_raw(" %-*s [%p]\n", col->func, buf, (void*)pc);
    return 0;
}

/* -------------------------------------------------------------------------- */
int backtrace_generate(struct backtrace_vec** bt)
{
    return backtrace_simple(bt_state, 0, bt_simple, bt_error, bt);
}

/* -------------------------------------------------------------------------- */
void backtrace_log_vec(const struct backtrace_vec* bt)
{
    const uintptr_t* pc;
    struct bt_columns col = {0, 0};
    vec_for_each (bt, pc)
        backtrace_pcinfo(bt_state, *pc, bt_full_calc_columns, bt_error, &col);
    vec_for_each (bt, pc)
        backtrace_pcinfo(bt_state, *pc, bt_full_print, bt_error, &col);
}

/* -------------------------------------------------------------------------- */
void backtrace_log(void)
{
    struct backtrace_vec* bt;
    const uintptr_t* pc;
    struct bt_columns col = {0, 0};
    backtrace_vec_init(&bt);
    backtrace_simple(bt_state, 0, bt_simple, bt_error, &bt);
    vec_for_each (bt, pc)
        backtrace_pcinfo(bt_state, *pc, bt_full_calc_columns, bt_error, &col);
    vec_for_each (bt, pc)
        backtrace_pcinfo(bt_state, *pc, bt_full_print, bt_error, &col);
    backtrace_vec_deinit(bt);
}

/* -------------------------------------------------------------------------- */
int backtrace_init(void)
{
    bt_state = backtrace_create_state(NULL, 0, NULL, NULL);
    if (bt_state == NULL)
        return -1;
    return 0;
}

/* -------------------------------------------------------------------------- */
void backtrace_deinit(void)
{
}
