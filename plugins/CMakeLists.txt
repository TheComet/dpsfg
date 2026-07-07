# Required so external plugins can make use of our build tree
configure_file ("cmake/modules/DPSFGPlugin.cmake" "${DPSFG_PREFIX}/share/dpsfg/cmake/modules/DPSFGPlugin.cmake" COPYONLY)
list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")
add_library (dpsfg-plugin INTERFACE)
target_include_directories (dpsfg-plugin INTERFACE
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>" # dpsfg-plugin.h
    "$<INSTALL_INTERFACE:include>")
add_library (DPSFG::plugin ALIAS dpsfg-plugin)

install (
    FILES "cmake/modules/DPSFGPlugin.cmake"
    DESTINATION "share/dpsfg/cmake/modules")

add_subdirectory ("bode-plot")
add_subdirectory ("filter-designer")
add_subdirectory ("graph-editor")
add_subdirectory ("math-viewer")
add_subdirectory ("pole-zero")
add_subdirectory ("substitutions")
add_subdirectory ("time-plot")
add_subdirectory ("tweakables")
