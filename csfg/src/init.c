#include "csfg/init.h"
#include "csfg/platform/backtrace.h"
#include "csfg/util/log.h"
#include "csfg/util/tracker.h"
#include "mathomatic/am.h"

/* -------------------------------------------------------------------------- */
int csfg_init(void)
{
    log_init();
    if (backtrace_init() != 0)
        goto init_backtrace_failed;
    if (trackers_init_tls() != 0)
        goto init_tracker_for_this_thread_failed;
    if (init_mem() == false)
        goto init_mathomatic_failed;

    return 0;

init_mathomatic_failed:
    trackers_deinit_tls();
init_tracker_for_this_thread_failed:
    backtrace_deinit();
init_backtrace_failed:
    return -1;
}

/* -------------------------------------------------------------------------- */
void csfg_deinit(void)
{
    free_mem();
    trackers_deinit_tls();
    backtrace_deinit();
}

/* -------------------------------------------------------------------------- */
int csfg_init_tls(void)
{
    return trackers_init_tls();
}

/* -------------------------------------------------------------------------- */
void csfg_deinit_tls(void)
{
    trackers_deinit_tls();
}
