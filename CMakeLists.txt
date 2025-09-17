cmake_minimum_required (VERSION 3.24)

# Force a debug build if not specified
if (NOT DEFINED CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build")
endif ()
message (STATUS "Build type: ${CMAKE_BUILD_TYPE}")

list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")

project ("dpsfg" LANGUAGES)

add_subdirectory ("ui")
