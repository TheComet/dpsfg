#include "gtest/gtest.h"

extern "C" int run_tests(int* argc, char* argv[]) {
    testing::InitGoogleTest(argc, argv);
    return RUN_ALL_TESTS();
}
