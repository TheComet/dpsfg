project ("dpsfg-ui"
  LANGUAGES C
  VERSION 0.0.1)

find_package (GTK4 REQUIRED COMPONENTS
    gtk glib cairo pango pangocairo harfbuzz gdk-pixbuf graphene)

gtk_compile_resource (
    "res/dpsfg-ui.gresource.xml"
    "src/dpsfg-ui.gresource.c")

add_executable (dpsfg-ui
    "res/dpsfg-ui.gresource.xml"
    "${PROJECT_BINARY_DIR}/src/dpsfg-ui.gresource.c"

    "src/args.c"
    "src/main.c")
target_include_directories (dpsfg-ui PRIVATE
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>")
target_link_libraries (dpsfg-ui PRIVATE
    csfg
    GTK4::glib
    GTK4::cairo
    GTK4::pango
    GTK4::pangocairo
    GTK4::harfbuzz
    GTK4::gdk-pixbuf
    GTK4::graphene
    GTK4::gtk)
set_target_properties (dpsfg-ui PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>)

if (DPSFG_TESTS)
    target_link_libraries (dpsfg-ui PRIVATE dpsfg-tests)
endif ()

if (WIN32 OR CYGWIN)
    add_custom_command (TARGET dpsfg-ui POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${GTK4_RUNTIMES} ${CMAKE_BINARY_DIR}/bin
        COMMENT "Copying GTK4 DLLs to ${CMAKE_BINARY_DIR}/bin"
        VERBATIM)
endif ()

install (
    TARGETS dpsfg-ui
    DESTINATION ".")

