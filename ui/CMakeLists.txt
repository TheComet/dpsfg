project ("dpsfg-ui"
  LANGUAGES C
  VERSION 0.0.1)

# Required so external plugins can make use of our build tree
configure_file ("cmake/modules/DPSFGPlugin.cmake" "${DPSFG_PREFIX}/share/dpsfg/cmake/modules/DPSFGPlugin.cmake" COPYONLY)
list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")
add_library (dpsfg-plugin INTERFACE)
target_include_directories (dpsfg-plugin INTERFACE
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:include>")
add_library (DPSFG::plugin ALIAS dpsfg-plugin)

find_package (GTK4 REQUIRED COMPONENTS
    gtk glib cairo pango pangocairo harfbuzz gdk-pixbuf graphene)

gtk_compile_resource (
    "res/dpsfg-ui.gresource.xml"
    "src/dpsfg-ui.gresource.c")

add_executable (dpsfg-ui
    "res/dpsfg-ui.gresource.xml"
    "${PROJECT_BINARY_DIR}/src/dpsfg-ui.gresource.c"

    "src/args.c"
    "src/dynlib.c"
    "src/fs.c"
    "src/plugin_loader.c"
    "src/main.c")
target_link_libraries (dpsfg-ui PRIVATE
    DPSFG::plugin
    DPSFG::csfg
    GTK4::glib
    GTK4::cairo
    GTK4::pango
    GTK4::pangocairo
    GTK4::harfbuzz
    GTK4::gdk-pixbuf
    GTK4::graphene
    GTK4::gtk)
target_compile_options (dpsfg-ui
    PRIVATE
        $<$<C_COMPILER_ID:MSVC>:/W4 /wd4706 /wd4305 /wd4244 /wd4505>
        $<$<C_COMPILER_ID:GNU>:-W -Wall -Wextra -pedantic -Wno-unused-function>
        $<$<C_COMPILER_ID:Clang>:-W -Wall -Wextra -pedantic -Wno-unused-function>)
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

if (WIN32 OR CYGWIN)
    add_custom_command (TARGET dpsfg-ui POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${GTK4_RUNTIMES} ${DPSFG_PREFIX}
        COMMENT "Copying GTK4 DLLs to ${DPSFG_PREFIX}"
        VERBATIM)
endif ()

install (
    TARGETS dpsfg-ui
    DESTINATION ".")
install (
    FILES "cmake/modules/DPSFGPlugin.cmake"
    DESTINATION "share/dpsfg/cmake/modules")

add_subdirectory ("plugins/graph-editor")
