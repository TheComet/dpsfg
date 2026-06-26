#pragma once

#include "csfg/config.h"
#include <stdint.h>

#if defined(_WIN32)
#   include <malloc.h>
static inline int mem_allocated_size(void* p) { return (int)_msize(p); }
#elif defined(__APPLE__)
#   include <malloc/malloc.h>
#   define mem_allocated_size  malloc_size
#else
#   include <malloc.h>
#   define mem_allocated_size  malloc_usable_size
#endif

#if !defined(CSFG_DEBUG_MEMORY)
/* clang-format off */
#   include <stdlib.h>
#   define mem_alloc                  malloc
#   define mem_free                   free
#   define mem_realloc                realloc
/* clang-format on */
#else

/*!
 * @brief Does the same thing as a normal call to malloc(), but does some
 * additional work to monitor and track down memory leaks.
 */
void* mem_alloc(int size);

/*!
 * @brief Does the same thing as a normal call to realloc(), but does some
 * additional work to monitor and track down memory leaks.
 */
void* mem_realloc(void* ptr, int new_size);

/*!
 * @brief Does the same thing as a normal call to fee(), but does some
 * additional work to monitor and track down memory leaks.
 */
void mem_free(void*);

#endif
