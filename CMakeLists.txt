cmake_minimum_required (VERSION 3.24)

if (NOT DEFINED CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE "Debug" CACHE STRING "")
endif ()
message (STATUS "Build type: ${CMAKE_BUILD_TYPE}")
list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")

project ("dpsfg" LANGUAGES)

option (DPSFG_TESTS "Build unit tests" ON)

set (DPSFG_PREFIX "${PROJECT_BINARY_DIR}/bin")

if (DPSFG_TESTS)
    enable_language (CXX)
    add_subdirectory ("tests")
endif ()

add_subdirectory ("csfg")
add_subdirectory ("ui")

# All subprojects have added targets to this export
# group, so make sure to install it
install (
    EXPORT DPSFGTargets
    NAMESPACE DPSFG::
    DESTINATION "share/dpsfg/cmake")
export (
    EXPORT DPSFGTargets
    NAMESPACE DPSFG::
    FILE "${DPSFG_PREFIX}/share/dpsfg/cmake/DPSFGTargets.cmake")

include(CMakePackageConfigHelpers)
configure_package_config_file("cmake/templates/DPSFGConfig.cmake.in"
    "${DPSFG_PREFIX}/share/dpsfg/cmake/DPSFGConfig.cmake"
    INSTALL_DESTINATION "share/dpsfg/cmake")
install(
    FILES "${DPSFG_PREFIX}/share/dpsfg/cmake/DPSFGConfig.cmake"
    DESTINATION "share/dpsfg/cmake")
