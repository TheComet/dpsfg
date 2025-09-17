project (dpsfg-tests
    LANGUAGES C CXX)

find_package (GTest)
if (NOT GTest_FOUND)
    set (INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    include (FetchContent)
    FetchContent_Declare (
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
        URL_HASH SHA256=65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c)
    FetchContent_MakeAvailable (googletest)
    set_target_properties (gmock PROPERTIES
        MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>)
endif ()

add_library (dpsfg-tests STATIC "src/run_tests.cpp")
target_link_libraries (dpsfg-tests PUBLIC GTest::gmock)
