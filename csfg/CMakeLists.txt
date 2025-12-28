project ("dpsfg-csfg"
    VERSION 0.0.1
    LANGUAGES C)

###############################################################################
# Config and options
###############################################################################

option (CSFG_BACKTRACE "Enable generating backtraces to all resources. Adds a significant performance penalty" ON)
option (CSFG_DEBUG_MEMORY "Enable tracking malloc() and free() calls to detect memory leaks" ON)
option (CSFG_LOG_DEBUG "Make log_dbg() do something" ON)
option (CSFG_WERROR "Enable -Werror" ON)

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
    # util
    "src/util/hash.c"
    "src/util/log.c"
    "src/util/str.c"
    "src/util/strlist.c"
    "src/util/strview.c"

    # io
    "src/io/deserialize.c"
    "src/io/serialize.c"
    "src/io/io.c"
    "src/io/binary.c"
    "src/io/graphviz.c"
    "src/io/tikz.c"

    # graph
    "src/graph/graph.c"
    "src/graph/graph_find_paths_and_loops.c"
    "src/graph/graph_find_nontouching.c"
    "src/graph/graph_mason.c"
    "src/graph/path.c"

    # symbolic
    "src/symbolic/expr.c"
    "src/symbolic/expr_eval.c"
    "src/symbolic/expr_integrity_check.c"
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
    "src/symbolic/expr_opt_remove_useless_ops.c"
    "src/symbolic/expr_opt_fold_constants.c"
    "src/symbolic/poly_expr.c"
    "src/symbolic/tf_expr.c"
    "src/symbolic/var_table.c"

    # numeric
    "src/numeric/mat.c"
    "src/numeric/mat_solve_linear_system_lu.c"
    "src/numeric/mat_lu_decomposition.c"
    "src/numeric/poly.c"
    "src/numeric/poly_find_roots.c"
    "src/numeric/poly_pfd.c"
    "src/numeric/poly_eval_inverse_laplace.c"
    "src/numeric/tf.c")

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
        "src/platform/mfile_linux.c")
    if (CSFG_BACKTRACE)
        list (APPEND csfg_SOURCES
            "src/platform/backtrace_linux.c")
    endif ()
elseif (UNIX)
    list (APPEND csfg_SOURCES
        "src/platform/mfile_posix.c")
    if (CSFG_BACKTRACE)
        message (FATAL_ERROR "backtraces are not yet implemented for this platform. Build with -DCSFG_BACKTRACE=OFF, or implement it yourself ;)")
    endif ()
endif ()

add_library (dpsfg-csfg SHARED
    ${csfg_SOURCES})
target_include_directories (dpsfg-csfg PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>"
    "$<INSTALL_INTERFACE:include>")
target_compile_options (dpsfg-csfg
    PRIVATE
        $<$<C_COMPILER_ID:MSVC>:/W4 /wd4706 /wd4305 /wd4244 /wd4505>
        $<$<C_COMPILER_ID:GNU>:-W -Wall -Wextra -pedantic -Wno-unused-function>
        $<$<C_COMPILER_ID:Clang>:-W -Wall -Wextra -pedantic -Wno-unused-function>)
set_target_properties (dpsfg-csfg
    PROPERTIES
        C_STANDARD 90)
add_library (DPSFG::csfg ALIAS dpsfg-csfg)

if (CSFG_WERROR)
    target_compile_options (dpsfg-csfg
        PRIVATE
            $<$<C_COMPILER_ID:GNU>:-Werror>
            $<$<C_COMPILER_ID:Clang>:-Werror>)
endif ()

if (CSFG_TESTS)
    target_sources (dpsfg-tests INTERFACE
        "tests/LogHelper.cpp"
        "tests/Printers.cpp"

        # symbolic
        "tests/test_expr.cpp"
        "tests/test_expr_apply_limits.cpp"
        "tests/test_expr_insert_substitutions.cpp"
        "tests/test_expr_op_distribute_products.cpp"
        "tests/test_expr_op_expand_constant_exponents.cpp"
        "tests/test_expr_op_expand_exponent_products.cpp"
        "tests/test_expr_op_expand_exponent_sums.cpp"
        "tests/test_expr_op_factor_common_denominator.cpp"
        "tests/test_expr_op_lower_negates.cpp"
        "tests/test_expr_op_rebalance_fraction.cpp"
        "tests/test_expr_op_simplify_products.cpp"
        "tests/test_expr_op_simplify_sums.cpp"
        "tests/test_expr_opt_remove_useless_ops.cpp"
        "tests/test_expr_opt_fold_constants.cpp"
        "tests/test_expr_to_rational.cpp"
        "tests/test_expr_to_rational_limit.cpp"
        "tests/test_expr_to_rational_limits.cpp"
        "tests/test_poly_add_permutations.cpp"
        "tests/test_tf_expr.cpp"
        "tests/test_tf_expr_simplify.cpp"

        # graph
        "tests/test_graph_find_forward_paths.cpp"
        "tests/test_graph_find_loops.cpp"
        "tests/test_graph_find_nontouching.cpp"
        "tests/test_graph_mason.cpp"

        # numeric
        "tests/test_complex.cpp"
        "tests/test_cpoly_eval.cpp"
        "tests/test_cpoly_find_roots.cpp"
        "tests/test_mat.cpp"
        "tests/test_mat_lu_decomposition.cpp"
        "tests/test_mat_solve_linear_system_lu.cpp"
        "tests/test_poly_eval_inverse_laplace.cpp"
        "tests/test_poly_pfd.cpp"
        "tests/test_tf_eval.cpp"

        # util
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
