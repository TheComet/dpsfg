project ("dpsfg-ui"
    LANGUAGES C
    VERSION 0.0.1)

find_package (SDL2 REQUIRED)
find_package (OpenGL REQUIRED COMPONENTS OpenGL)

sqlgen_target (dpsfg-db
    INPUT "src/db.sqlgen"
    HEADER "include/ui/db.h"
    BACKENDS sqlite3)

add_executable (dpsfg-ui
    ${SQLGEN_dpsfg-db_OUTPUTS}
    "src/args.c"
    "src/db.sqlgen"
    "src/dynlib.c"
    "src/fs.c"
    "src/math_pipeline.c"
    "src/microui.c"
    "src/plugin_loader.c"
    "src/project_browser.c"
    "src/renderer.c"
    "src/main.c")
target_include_directories (dpsfg-ui PRIVATE
    "${PROJECT_BINARY_DIR}/include"
    "${PROJECT_SOURCE_DIR}/include")
target_compile_options (dpsfg-ui PRIVATE
    $<$<C_COMPILER_ID:GNU>:-W -Wall -Wextra -pedantic -Wno-unused-function -Wno-overlength-strings>
    $<$<C_COMPILER_ID:Clang>:-W -Wall -Wextra -pedantic -Wno-unused-function -Wno-overlength-strings>)
target_link_libraries (dpsfg-ui PRIVATE
    DPSFG::csfg
    DPSFG::plugin
    SDL2::SDL2
    OpenGL::OpenGL
    sqlite)

set_target_properties (dpsfg-ui PROPERTIES
    C_STANDARD 90
    RUNTIME_OUTPUT_DIRECTORY ${DPSFG_PREFIX}
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${DPSFG_PREFIX}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${DPSFG_PREFIX}
    VS_DEBUGGER_WORKING_DIRECTORY ${DPSFG_PREFIX}
    MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>)

if (DPSFG_TESTS)
    target_link_libraries (dpsfg-ui PRIVATE dpsfg-tests)
endif ()

install (
    TARGETS dpsfg-ui
    DESTINATION ".")
