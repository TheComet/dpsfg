project ("mathomatic"
    VERSION 0.0.1
    LANGUAGES C)

###############################################################################
# Config and options
###############################################################################

file (STRINGS "${PROJECT_SOURCE_DIR}/VERSION" MATHOMATIC_VERSION)

set (MATHOMATIC_NOT80COLUMNS 0 CACHE STRING "For programs that use less than 80 columns wide display")
set (MATHOMATIC_HELP 1 CACHE STRING "Define HELP=0 to remove the help command. Shaves at least 10 kilobytes off of the executable.")
set (MATHOMATIC_LIBRARY 1 CACHE STRING "Build as a symbolic math library instead of a command-line executable")
set (MATHOMATIC_SILENT 1 CACHE STRING "Define SILENT=0 to have debugging information printed to stdout")
set (MATHOMATIC_READLINE 0 CACHE STRING "Enable readline editing and history support (larger than editline)")
set (MATHOMATIC_EDITLINE 0 CACHE STRING "Enable editline editing and history support (smaller than readline)")
set (MATHOMATIC_SHOW_RESOURCES 0 CACHE STRING "Use getrusage() to print resource usage statistics of the program")
set (MATHOMATIC_I18N 0 CACHE STRING "Enable internationalization. This doesn't work yet. It would need a translation and some work on the code and buildsystem.")
set (MATHOMATIC_DBL_DIG 15 CACHE STRING "The number of digits of precision of doubles")
set (MATHOMATIC_HANDHELD 0 CACHE STRING "Enable this option if compiling for embedded or small devices. Reduces memory usage in a few places")

configure_file ("templates/config.h.in" "include/mathomatic/config.h")

###############################################################################
# Library
###############################################################################

add_library (mathomatic OBJECT
    "src/am.c"
    "src/cmds.c"
    "src/complex.c"
    "src/complex_lib.c"
    "src/diff.c"
    "src/factor.c"
    "src/factor_int.c"
    "src/gcd.c"
    "src/globals.c"
    "src/help.c"
    "src/integrate.c"
    "src/list.c"
    "src/parse.c"
    "src/poly.c"
    "src/simplify.c"
    "src/solve.c"
    "src/super.c"
    "src/unfactor.c")
target_include_directories (mathomatic PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>"
    "$<INSTALL_INTERFACE:include>")
target_compile_options (mathomatic
    PRIVATE
        $<$<C_COMPILER_ID:GNU>:-W -Wall -Wextra -Wno-unused-parameter -Wno-unused-but-set-variable -Wno-missing-field-initializers>
        $<$<C_COMPILER_ID:Clang>:-W -Wall -Wextra>)
set_target_properties (mathomatic
    PROPERTIES
        C_STANDARD 90
        POSITION_INDEPENDENT_CODE ON)
add_library (DPSFG::mathomatic ALIAS mathomatic)
