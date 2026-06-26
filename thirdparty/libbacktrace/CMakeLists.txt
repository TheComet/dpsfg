project ("libbacktrace" LANGUAGES C)

add_library (libbacktrace OBJECT
    "src/atomic.c"
    "src/backtrace.c"
    "src/dwarf.c"
    "src/elf.c"
    "src/fileline.c"
    "src/mmapio.c"
    "src/mmap.c"
    "src/posix.c"
    "src/print.c"
    "src/simple.c"
    "src/sort.c"
    "src/state.c")
target_include_directories (libbacktrace
    PRIVATE
        "${PROJECT_SOURCE_DIR}/src"
    PUBLIC
        "${PROJECT_SOURCE_DIR}/include")
set_target_properties (libbacktrace PROPERTIES
    POSITION_INDEPENDENT_CODE ON)
