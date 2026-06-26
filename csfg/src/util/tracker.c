#include "csfg/util/backtrace.h"
#include "csfg/util/hash.h"
#include "csfg/util/hmap.h"
#include "csfg/util/mem.h"
#include "csfg/util/tracker.h"
#include <inttypes.h>

struct data
{
    int size;
    char name[32];
    struct backtrace_vec* backtrace;
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
    char name[32];
    struct tracker_hmap* hmap;
    int tracks, untracks;
};

/* -------------------------------------------------------------------------- */
struct tracker* tracker_create(const char* name)
{
    struct tracker* t = mem_alloc(sizeof *t);
    if (t == NULL)
        return NULL;

    tracker_hmap_init(&t->hmap);
    strncpy(t->name, name, sizeof(t->name) - 1);

    t->tracks   = 0;
    t->untracks = 0;

    return t;
}

/* -------------------------------------------------------------------------- */
void tracker_destroy(struct tracker* t)
{
    int slot;
    uintptr_t p;
    struct data* data;

    hmap_for_each (t->hmap, slot, p, data)
    {
        log_err("Un-freed %s 0x%" PRIxPTR " \"%s\"", t->name, p, data->name);
        if (data->size)
            log_raw(", size %d", data->size);
        log_raw("\n");
        backtrace_log_vec(data->backtrace);
        backtrace_vec_deinit(data->backtrace);
    }

    tracker_hmap_deinit(t->hmap);
    mem_free(t);
}

/* -------------------------------------------------------------------------- */
void tracker_track(struct tracker* t, void* p, int size, const char* name)
{
    struct data* data;
    ++t->tracks;

    switch (tracker_hmap_emplace_or_get(&t->hmap, (uintptr_t)p, &data))
    {
        case HMAP_OOM: break;
        case HMAP_NEW: {
            strncpy(data->name, name, sizeof(data->name) - 1);
            data->size      = size;
            data->backtrace = NULL;
            backtrace_generate(&data->backtrace);
            break;
        }

        case HMAP_EXISTS: {
            log_err(
                "Double track on %s! This is usually caused by "
                "calling tracker_track() on the same resource twice.\n",
                t->name);
            backtrace_log();
            log_note("%s was previously tracked at:\n", t->name);
            backtrace_log_vec(data->backtrace);
            break;
        }
    }
}

/* -------------------------------------------------------------------------- */
void tracker_untrack(struct tracker* t, void* p)
{
    struct data* data;
    ++t->untracks;

    data = tracker_hmap_erase(t->hmap, (uintptr_t)p);
    if (data != NULL)
        backtrace_vec_deinit(data->backtrace);
    else
    {
        log_err("Untracking a %s that was never tracked.\n", t->name);
        backtrace_log();
    }
}

/* -------------------------------------------------------------------------- */
static CSFG_THREADLOCAL int g_ignore_malloc;
static CSFG_THREADLOCAL struct tracker* g_tracker_mem;
static CSFG_THREADLOCAL struct tracker* g_tracker_fd;

/* -------------------------------------------------------------------------- */
int trackers_init_tls(void)
{
    g_ignore_malloc = 1;
    g_tracker_mem   = tracker_create("memory allocation");
    if (g_tracker_mem == NULL)
        goto tracker_mem_create_failed;
    g_ignore_malloc = 0;
    track_mem(g_tracker_mem, sizeof *g_tracker_mem, "g_tracker_mem");

    g_tracker_fd = tracker_create("file descriptor");
    if (g_tracker_fd == NULL)
        goto tracker_fd_create_failed;

    return 0;

tracker_fd_create_failed:
    tracker_destroy(g_tracker_mem);
tracker_mem_create_failed:
    return -1;
}

/* -------------------------------------------------------------------------- */
void trackers_deinit_tls(void)
{
    tracker_destroy(g_tracker_fd);

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
