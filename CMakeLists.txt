cmake_minimum_required (VERSION 3.24)

if (NOT DEFINED CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE "Debug" CACHE STRING "")
endif ()
message (STATUS "Build type: ${CMAKE_BUILD_TYPE}")
list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")

project ("dpsfg" LANGUAGES)

option (DPSFG_TESTS "Build unit tests" ON)

if (DPSFG_TESTS)
    enable_language (CXX)
    add_subdirectory ("tests")
endif ()

add_subdirectory ("csfg")
add_subdirectory ("ui")

