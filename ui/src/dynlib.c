#include "csfg/util/log.h"
#include "csfg/util/tracker.h"
#include "dpsfg/dynlib.h"

#if defined(_WIN32)
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>
#else
#    define _GNU_SOURCE
#    include <dlfcn.h>
#    include <elf.h>
#    include <link.h>
#endif

#include <stddef.h>

/* -------------------------------------------------------------------------- */
int dynlib_add_path(const char* path)
{
#if defined(_WIN32)
    /* This function does not appear to add duplicates so it's safe to call it
     * multiple times */
    if (!SetDllDirectoryA(path))
        return -1;
    return 0;

#else
    (void)path;
    return 0;
#endif
}

/* -------------------------------------------------------------------------- */
void* dynlib_open(const char* filename)
{
#if defined(_WIN32)
    void* handle = (void*)LoadLibraryA(filename);
#else
    void* handle = dlopen(filename, RTLD_LAZY);
    if (handle == NULL)
    {
        log_err(
            "Failed to load shared library \"%s\": %s\n", filename, dlerror());
        return NULL;
    }
#endif
    track_mem(handle, 0, filename);
    return handle;
}

/* -------------------------------------------------------------------------- */
void dynlib_close(void* handle)
{
    untrack_mem(handle);
#if defined(_WIN32)
    FreeLibrary((HMODULE)handle);
#else
    dlclose(handle);
#endif
}

/* -------------------------------------------------------------------------- */
void* dynlib_symbol_addr(void* handle, const char* name)
{
#if defined(_WIN32)
    return (void*)GetProcAddress((HMODULE)handle, name);
#else
    return dlsym(handle, name);
#endif
}
