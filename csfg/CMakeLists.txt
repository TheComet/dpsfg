project ("dpsfg-csfg"
    VERSION 0.0.1
    LANGUAGES C)

###############################################################################
# Config and options
###############################################################################

option (CSFG_BACKTRACE "Enable generating backtraces to all resources. Adds a significant performance penalty" ON)
option (CSFG_DEBUG_MEMORY "Enable tracking malloc() and free() calls to detect memory leaks" ON)
option (CSFG_LOG_DEBUG "Make log_dbg() do something" ON)

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

    "src/symbolic/expr.c"
    "src/symbolic/expr_eval.c"
    "src/symbolic/expr_parse.c"
    "src/symbolic/expr_op.c"
    "src/symbolic/expr_op_distribute_products.c"
    "src/symbolic/expr_op_expand_constant_exponents.c"
    "src/symbolic/expr_op_expand_exponent_products.c"
    "src/symbolic/expr_op_expand_exponent_sums.c"
    "src/symbolic/expr_op_factor_common_denominator.c"
    "src/symbolic/expr_op_lower_negates.c"
    "src/symbolic/expr_op_rebalance_fraction.c"
    "src/symbolic/expr_op_simplify_products.c"
    "src/symbolic/expr_op_simplify_sums.c"
    "src/symbolic/expr_op_tf.c"
    "src/symbolic/expr_opt_remove_useless_ops.c"
    "src/symbolic/expr_opt_fold_constants.c"
    "src/symbolic/var_table.c")

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

add_library (dpsfg-csfg SHARED
    ${csfg_SOURCES})
target_include_directories (dpsfg-csfg PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>"
    "$<INSTALL_INTERFACE:include>")
add_library (DPSFG::csfg ALIAS dpsfg-csfg)

if (CSFG_TESTS)
    target_sources (dpsfg-tests INTERFACE
        "tests/LogHelper.cpp"
        "tests/test_expr.cpp"
        "tests/test_expr_op_distribute_products.cpp"
        "tests/test_expr_op_expand_constant_exponents.cpp"
        "tests/test_expr_op_expand_exponent_products.cpp"
        "tests/test_expr_op_expand_exponent_sums.cpp"
        "tests/test_expr_op_factor_common_denominator.cpp"
        "tests/test_expr_op_lower_negates.cpp"
        "tests/test_expr_op_rebalance_fraction.cpp"
        "tests/test_expr_op_simplify_products.cpp"
        "tests/test_expr_op_simplify_sums.cpp"
        "tests/test_expr_op_tf.cpp"
        "tests/test_expr_opt_remove_useless_ops.cpp"
        "tests/test_expr_opt_fold_constants.cpp"

        "tests/test_hmap.cpp"
        "tests/test_hmap_full.cpp"
        "tests/test_hset.cpp"
        "tests/test_mem.cpp"
        "tests/test_strlist.cpp"
        "tests/test_strview.cpp"
        "tests/test_vec.cpp")
    target_include_directories (dpsfg-tests INTERFACE
        "${PROJECT_SOURCE_DIR}/tests/include")
endif ()

install (
    TARGETS dpsfg-csfg
    EXPORT DPSFGTargets
    INCLUDES DESTINATION "include"
    ARCHIVE DESTINATION "lib"
    LIBRARY DESTINATION "lib"
    RUNTIME DESTINATION ".")
install (
    DIRECTORY "include/csfg"
    DESTINATION "include")
install (
    DIRECTORY "${PROJECT_BINARY_DIR}/include/csfg"
    DESTINATION "include")
