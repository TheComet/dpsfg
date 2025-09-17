#include "csfg/platform/backtrace.h"
#include "csfg/util/hmap.h"
#include "csfg/util/mem.h"
#include "csfg/util/str.h"
#include "csfg/util/tracker.h"
#include <inttypes.h>

struct data
{
    int         size;
    struct str* name;
#if defined(CSFG_BACKTRACE)
    int    backtrace_size;
    char** backtrace;
#endif
};

HMAP_DECLARE_HASH(static, tracker_hmap, hash32, uintptr_t, struct data, 32)
HMAP_DEFINE_HASH(
    static,
    tracker_hmap,
    hash32,
    uintptr_t,
    struct data,
    32,
    hash32_aligned_ptr)

struct tracker
{
    struct str*          name;
    struct tracker_hmap* hmap;
    int                  tracks, untracks;
};

/* ------------------------------------------------------------------------- */
#if defined(CSFG_BACKTRACE)
static void print_backtrace(const struct data* data)
{
    int i;
    for (i = BACKTRACE_OMIT_COUNT; i < data->backtrace_size; ++i)
    {
        if (strstr(data->backtrace[i], "invoke_main"))
            break;
        log_raw("  %s\n", data->backtrace[i]);
    }
}
#endif

/* ------------------------------------------------------------------------- */
struct tracker* tracker_create(const char* name)
{
    struct tracker* t = mem_alloc(sizeof *t);
    if (t == NULL)
        goto alloc_tracker_failed;

    str_init(&t->name);
    tracker_hmap_init(&t->hmap);

    if (str_set_cstr(&t->name, name) != 0)
        goto set_name_failed;

    t->tracks = 0;
    t->untracks = 0;

    return t;

set_name_failed:
    tracker_hmap_deinit(t->hmap);
    str_deinit(t->name);
    mem_free(t);
alloc_tracker_failed:
    return NULL;
}

/* ------------------------------------------------------------------------- */
void tracker_destroy(struct tracker* t)
{
    int          slot;
    uintptr_t    p;
    struct data* data;

    hmap_for_each (t->hmap, slot, p, data)
    {
        (void)slot;
        log_err(
            "Un-freed %s 0x%" PRIxPTR " \"%s\"",
            str_cstr(t->name),
            p,
            str_cstr(data->name));
        if (data->size)
            log_raw(", size %d", data->size);
        log_raw("\n");
#if defined(CSFG_BACKTRACE)
        print_backtrace(data);
        backtrace_free(data->backtrace);
#endif
        str_deinit(data->name);
#if defined(CSFG_HEX_DUMP)
        if (data->size <= CSFG_HEX_DUMP_SIZE)
            log_hex_ascii((void*)p, data->size);
#endif
    }

#if defined(CSFG_BACKTRACE)
    if (t->tracks != t->untracks)
    {
        log_note("Call to tracker_destroy():\n");
        log_backtrace();
    }
#endif

    tracker_hmap_deinit(t->hmap);
    str_deinit(t->name);
    mem_free(t);
}

/* ------------------------------------------------------------------------- */
void tracker_track(struct tracker* t, void* p, int size, const char* name)
{
    struct data* data;
    ++t->tracks;

    switch (tracker_hmap_emplace_or_get(&t->hmap, (uintptr_t)p, &data))
    {
        case HMAP_OOM: break;
        case HMAP_NEW: {
            str_init(&data->name);
            str_set_cstr(&data->name, name);
            data->size = size;
#if defined(CSFG_BACKTRACE)
            data->backtrace = backtrace_get(&data->backtrace_size);
#endif
            break;
        }

        case HMAP_EXISTS: {
            log_err(
                "Double track on %s! This is usually caused by "
                "calling tracker_track() on the same resource twice.\n",
                str_cstr(t->name));
#if defined(CSFG_BACKTRACE)
            log_backtrace();
            log_err("%s was previously tracked at:\n", str_cstr(t->name));
            print_backtrace(data);
#endif
            break;
        }
    }
}

/* ------------------------------------------------------------------------- */
void tracker_untrack(struct tracker* t, void* p)
{
    struct data* data;
    ++t->untracks;

    data = tracker_hmap_erase(t->hmap, (uintptr_t)p);
    if (data == NULL)
    {
        log_err("Untracking a %s that was never tracked.\n", str_cstr(t->name));
#if defined(CSFG_BACKTRACE)
        log_backtrace();
#endif
        return;
    }

#if defined(CSFG_BACKTRACE)
    if (data->backtrace)
        backtrace_free(data->backtrace);
#endif
    str_deinit(data->name);
}

/* ------------------------------------------------------------------------- */
static CSFG_THREADLOCAL int             g_ignore_malloc;
static CSFG_THREADLOCAL struct tracker* g_tracker_mem;
static CSFG_THREADLOCAL struct tracker* g_tracker_fd;

/* ------------------------------------------------------------------------- */
int trackers_init_tls(void)
{
    g_ignore_malloc = 1;
    g_tracker_mem = tracker_create("memory allocation");
    if (g_tracker_mem == NULL)
        goto tracker_mem_create_failed;
    g_ignore_malloc = 0;
    track_mem(g_tracker_mem, sizeof *g_tracker_mem, "g_tracker_mem");
    track_mem(
        g_tracker_mem->name,
        str_len(g_tracker_mem->name),
        "g_tracker_mem->name");

    g_tracker_fd = tracker_create("file descriptor");
    if (g_tracker_fd == NULL)
        goto tracker_fd_create_failed;

    return 0;

tracker_fd_create_failed:
    tracker_destroy(g_tracker_mem);
tracker_mem_create_failed:
    return -1;
}

/* ------------------------------------------------------------------------- */
void trackers_deinit_tls(void)
{
    tracker_destroy(g_tracker_fd);

    untrack_mem(g_tracker_mem->name);
    untrack_mem(g_tracker_mem);
    g_ignore_malloc = 1;
    tracker_destroy(g_tracker_mem);
    g_ignore_malloc = 0;
}

void track_mem(void* p, int size, const char* name)
{
    if (!g_ignore_malloc)
    {
        g_ignore_malloc = 1;
        tracker_track(g_tracker_mem, p, size, name);
        g_ignore_malloc = 0;
    }
}
void track_fd(int fd, const char* name)
{
    tracker_track(g_tracker_fd, (void*)(intptr_t)fd, 0, name);
}

void untrack_mem(void* p)
{
    if (!g_ignore_malloc)
    {
        g_ignore_malloc = 1;
        tracker_untrack(g_tracker_mem, p);
        g_ignore_malloc = 0;
    }
}
void untrack_fd(int fd)
{
    tracker_untrack(g_tracker_fd, (void*)(intptr_t)fd);
}
