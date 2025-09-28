function (dpsfg_add_plugin NAME)
    string (TOUPPER ${NAME} PLUGIN_NAME)
    string (REPLACE "-" "_" PLUGIN_NAME ${PLUGIN_NAME})
    string (REPLACE " " "_" PLUGIN_NAME ${PLUGIN_NAME})

    string (TOLOWER ${NAME} TARGET_NAME)
    string (REPLACE "_" "-" TARGET_NAME ${TARGET_NAME})
    string (REPLACE " " "-" TARGET_NAME ${TARGET_NAME})

    set (options "")
    set (oneValueArgs "")
    set (multiValueArgs
        SOURCES
        TESTS
        HEADERS
        DEFINES
        INCLUDES
        LIBS
        DATA)
    cmake_parse_arguments (${PLUGIN_NAME} "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    option (${PLUGIN_NAME} "Enable the ${TARGET_NAME} plugin" ON)

    if (${${PLUGIN_NAME}})
        if ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
            set (PLUGIN_API "__declspec(dllexport)")
        elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
            set (PLUGIN_API "__attribute__((visibility(\"default\")))")
        else ()
            message (FATAL_ERROR "Don't know how to define visibility macros for this compiler")
        endif ()

        message (STATUS "Adding plugin: ${TARGET_NAME}")
        add_library (${TARGET_NAME} SHARED
            ${${PLUGIN_NAME}_SOURCES}
            ${${PLUGIN_NAME}_HEADERS})
        target_include_directories (${TARGET_NAME}
            PRIVATE
                ${${PLUGIN_NAME}_INCLUDES}
                "${DPSFG_PREFIX}/include")
        target_compile_definitions (${TARGET_NAME}
            PRIVATE
                ${${PLUGIN_NAME}_DEFINES}
                PLUGIN_BUILDING
                "PLUGIN_VERSION=((${PROJECT_VERSION_MAJOR}<<24) | (${PROJECT_VERSION_MINOR}<<16) | (${PROJECT_VERSION_PATCH}<<8))"
                "PLUGIN_API=${PLUGIN_API}")
        target_link_libraries (${TARGET_NAME}
            PRIVATE
                ${${PLUGIN_NAME}_LIBS})
        set_target_properties (${TARGET_NAME}
            PROPERTIES
                PREFIX ""
                MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>
                LIBRARY_OUTPUT_DIRECTORY
                    "${DPSFG_PREFIX}/share/dpsfg/plugins/${TARGET_NAME}"
                LIBRARY_OUTPUT_DIRECTORY_DEBUG
                    "${DPSFG_PREFIX}/share/dpsfg/plugins/${TARGET_NAME}"
                LIBRARY_OUTPUT_DIRECTORY_RELEASE
                    "${DPSFG_PREFIX}/share/dpsfg/plugins/${TARGET_NAME}"
                RUNTIME_OUTPUT_DIRECTORY
                    "${DPSFG_PREFIX}/share/dpsfg/plugins/${TARGET_NAME}"
                RUNTIME_OUTPUT_DIRECTORY_DEBUG
                    "${DPSFG_PREFIX}/share/dpsfg/plugins/${TARGET_NAME}"
                RUNTIME_OUTPUT_DIRECTORY_RELEASE
                    "${DPSFG_PREFIX}/share/dpsfg/plugins/${TARGET_NAME}"
                INSTALL_RPATH "lib"
                VS_DEBUGGER_WORKING_DIRECTORY "${DPSFG_PREFIX}"
                VS_DEBUGGER_COMMAND "${DPSFG_PREFIX}/dpsfg.exe")
        set_property (
            DIRECTORY "${PROJECT_SOURCE_DIR}"
            PROPERTY VS_STARTUP_PROJECT ${TARGET_NAME})
        if (TARGET dpsfg-ui)
            add_dependencies (dpsfg-ui ${TARGET_NAME})
        endif ()
        install (
            TARGETS ${TARGET_NAME}
            LIBRARY DESTINATION "share/dpsfg/plugins/${TARGET_NAME}/"
            RUNTIME DESTINATION "share/dpsfg/plugins/${TARGET_NAME}/")
    endif ()
endfunction ()
