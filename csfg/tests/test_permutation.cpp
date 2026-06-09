#include <vector>

#include "gtest/gtest.h"

extern "C" {
#include "csfg/util/permutation.h"
}

namespace {

struct TestParamInputOutput
{
    std::initializer_list<int> input;
    std::initializer_list<int> expected_output;
    int expected_return_value;
};

struct TestParamFullCycle
{
    const std::initializer_list<int> input;
    int expected_number_of_permutations;
};

const struct TestParamInputOutput TEST_PARAMS_INPUT_OUTPUT[] = {
    // clang-format off
    {       {},        {}, 0},
    {      {1},       {1}, 0},
    {    {1,2},     {2,1}, 1},
    {    {2,1},     {1,2}, 0},
    {{4,3,2,1}, {1,2,3,4}, 0},
    {{1,2,3,4}, {2,1,3,4}, 1},
    {{1,2,2,1}, {2,1,2,1}, 1},
    {{2,2,1,1}, {1,1,2,2}, 0},
    // clang-format on
};

const struct TestParamFullCycle TEST_PARAMS_FULL_CYCLE[] = {
    // clang-format off
    {         {},               1},
    {        {1},               1},
    {      {1,1},               1},
    {    {1,1,1},               1},
    {  {1,1,1,1},               1},
    {      {1,2},             1*2},
    {    {1,2,3},           1*2*3},
    {  {1,2,3,4},         1*2*3*4},
    {{1,2,3,4,5},       1*2*3*4*5},
    {  {1,1,2,2},   1*2*3*4/(2*2)},
    {  {1,1,2,3}, 1*2*3*4/(2*1*1)},
    // clang-format on
};

std::ostream& operator<<(std::ostream& os, std::initializer_list<int> l)
{
    os << "{";
    bool first = true;
    for (int value : l)
    {
        os << (first ? "" : ",") << value;
        first = false;
    }
    os << "}";
    return os;
}

std::ostream& operator<<(std::ostream& os, const std::vector<int>& p)
{
    os << "{";
    bool first = true;
    for (int value : p)
    {
        os << (first ? "" : ",") << value;
        first = false;
    }
    os << "}";
    return os;
}

std::ostream& operator<<(std::ostream& os, const struct TestParamInputOutput& p)
{
    os << p.input << " --> " << p.expected_output;
    return os;
}

std::ostream& operator<<(std::ostream& os, const struct TestParamFullCycle& p)
{
    os << p.input;
    return os;
}

bool ListsEq(
    const std::vector<int>& actual, std::initializer_list<int> expected)
{
    int i = 0;
    if (actual.size() != expected.size())
    {
        std::cerr << "Expected: " << expected << std::endl;
        std::cerr << "Actual  : " << actual << std::endl;
        return false;
    }
    for (int value : expected)
    {
        if (actual[i] != value)
        {
            std::cerr << "Expected: " << expected << std::endl;
            std::cerr << "Actual  : " << actual << std::endl;
            return false;
        }
        i++;
    }
    return true;
}

} // namespace

using namespace ::testing;

// clang-format off
struct test_permutation_input_output : TestWithParam<TestParamInputOutput>{};
struct test_permutation_full_cycle : TestWithParam<TestParamFullCycle>{};
struct test_permutation_sort : Test{};
// clang-format on

INSTANTIATE_TEST_SUITE_P(
    , test_permutation_input_output, ValuesIn(TEST_PARAMS_INPUT_OUTPUT));
INSTANTIATE_TEST_SUITE_P(
    , test_permutation_full_cycle, ValuesIn(TEST_PARAMS_FULL_CYCLE));

TEST_P(test_permutation_input_output, test)
{
    std::vector<int> p = GetParam().input;

    EXPECT_EQ(
        permutation_next(p.data(), p.size()), GetParam().expected_return_value);

    EXPECT_TRUE(ListsEq(p, GetParam().expected_output));
}

TEST_P(test_permutation_full_cycle, test)
{
    std::vector<int> p = GetParam().input;

    int permutations = 1;
    while (permutation_next(p.data(), p.size()))
        permutations++;

    EXPECT_EQ(permutations, GetParam().expected_number_of_permutations);
    EXPECT_TRUE(ListsEq(p, GetParam().input));
}

TEST_F(test_permutation_sort, test)
{
    std::vector<int> p = {3, 1, 2, 4};

    permutation_sort(p.data(), p.size());
    int permutations = 1;
    while (permutation_next(p.data(), p.size()))
        permutations++;

    EXPECT_EQ(permutations, 24);
    EXPECT_TRUE(ListsEq(p, {1, 2, 3, 4}));
}
