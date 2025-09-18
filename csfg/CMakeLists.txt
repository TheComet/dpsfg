project ("csfg"
    VERSION 0.0.1
    LANGUAGES C)

###############################################################################
# Config and options
###############################################################################

option (CSFG_DEBUG_MEMORY "Enable tracking malloc() and free() calls to detect memory leaks" ON)
option (CSFG_BACKTRACE "Enable generating backtraces to all resources. Adds a significant performance penalty" ON)

if ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
    set (CSFG_THREADLOCAL "__declspec(thread)")
elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
    set (CSFG_THREADLOCAL "__thread")
else ()
    message (FATAL_ERROR "Builtins not yet implemented for compiler: ${CMAKE_C_COMPILER_ID}\nYou could try to add them yourself (just check CMakeLists.txt at this location)")
endif ()

if (DPSFG_TESTS)
    set (CSFG_TESTS ON)
endif ()

configure_file ("templates/config.h.in" "include/csfg/config.h")

set (csfg_SOURCES
    "src/init.c"
    "src/util/hash.c"
    "src/util/log.c"
    "src/util/str.c"
    "src/util/strlist.c"
    "src/util/strview.c"

    "src/symbolic/expr.c")

if (CSFG_DEBUG_MEMORY)
    list (APPEND csfg_SOURCES
        "src/util/mem.c"
        "src/util/tracker.c")
endif ()

if (WIN32)
    list (APPEND csfg_SOURCES
        "src/platform/mfile_win32.c")
    if (CSFG_BACKTRACE)
        list (APPEND csfg_SOURCES
            "src/platform/backtrace_win32.c")
    endif ()
elseif (LINUX)
    list (APPEND csfg_SOURCES
        "src/platform/mfile_posix.c")
    if (CSFG_BACKTRACE)
        list (APPEND csfg_SOURCES
            "src/platform/backtrace_linux.c")
    endif ()
endif ()

add_library (csfg STATIC
    ${csfg_SOURCES})
target_include_directories (csfg PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_BINARY_DIR}/include")

if (CSFG_TESTS)
    target_sources (dpsfg-tests INTERFACE
        "tests/test_expr.cpp")
endif ()

