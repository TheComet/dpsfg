#include "gmock/gmock.h"

extern "C" {
#include "csfg/numeric/complex.h"
}

#define NAME test_complex

using namespace testing;

namespace {
MATCHER_P2(
    ComplexEq,
    expected_real,
    expected_imag,
    "matches a csfg_complex approximately")
{
    return ExplainMatchResult(
        AllOf(
            Field(&csfg_complex::real, DoubleEq(expected_real)),
            Field(&csfg_complex::imag, DoubleEq(expected_imag))),
        arg,
        result_listener);
}
} // namespace

struct NAME : public Test
{
    void SetUp() override {}
    void TearDown() override {}

    struct csfg_complex z;
    struct csfg_complex z1;
    struct csfg_complex z2;
};

TEST_F(NAME, construct)
{
    z = csfg_complex(2.0, 5.0);
    ASSERT_DOUBLE_EQ(z.real, 2.0);
    ASSERT_DOUBLE_EQ(z.imag, 5.0);
}

TEST_F(NAME, mag)
{
    z = csfg_complex(2.0, 5.0);
    ASSERT_DOUBLE_EQ(csfg_complex_mag(z), sqrt(2 * 2 + 5 * 5));
}

TEST_F(NAME, add)
{
    z1 = csfg_complex(2.0, 5.0);
    z2 = csfg_complex(4.0, 3.0);
    ASSERT_THAT(csfg_complex_add(z1, z2), ComplexEq(6.0, 8.0));
}

TEST_F(NAME, mul)
{
    z1 = csfg_complex(2.0, 5.0);
    z2 = csfg_complex(4.0, 3.0);
    ASSERT_THAT(csfg_complex_mul(z1, z2), ComplexEq(-7.0, 26.0));
}

TEST_F(NAME, div)
{
    z1 = csfg_complex(2.0, 5.0);
    z2 = csfg_complex(4.0, 3.0);
    ASSERT_THAT(csfg_complex_div(z1, z2), ComplexEq(0.92, 0.56));
}
