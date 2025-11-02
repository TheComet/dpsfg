#include "gmock/gmock.h"

extern "C" {
#include "csfg/numeric/poly.h"
}

#define NAME test_cpoly_eval

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
    void SetUp() override { csfg_cpoly_init(&p); }
    void TearDown() override { csfg_cpoly_deinit(p); }

    struct csfg_cpoly*  p;
    struct csfg_complex z;
};

TEST_F(NAME, constant_real)
{
    csfg_cpoly_push(&p, csfg_complex(3, 0));
    ASSERT_THAT(csfg_cpoly_eval(p, csfg_complex(0, 0)), ComplexEq(3, 0));
    ASSERT_THAT(csfg_cpoly_eval(p, csfg_complex(1, 0)), ComplexEq(3, 0));
    ASSERT_THAT(csfg_cpoly_eval(p, csfg_complex(5, 0)), ComplexEq(3, 0));
}

TEST_F(NAME, constant_imag)
{
    csfg_cpoly_push(&p, csfg_complex(3, 0));
    ASSERT_THAT(csfg_cpoly_eval(p, csfg_complex(0, 0)), ComplexEq(3, 0));
    ASSERT_THAT(csfg_cpoly_eval(p, csfg_complex(0, 1)), ComplexEq(3, 0));
    ASSERT_THAT(csfg_cpoly_eval(p, csfg_complex(0, 5)), ComplexEq(3, 0));
}

TEST_F(NAME, linear_real)
{
    /* 3 + 2*s */
    csfg_cpoly_push(&p, csfg_complex(3, 0));
    csfg_cpoly_push(&p, csfg_complex(2, 0));
    ASSERT_THAT(csfg_cpoly_eval(p, csfg_complex(0, 0)), ComplexEq(3, 0));
    ASSERT_THAT(csfg_cpoly_eval(p, csfg_complex(1, 0)), ComplexEq(5, 0));
    ASSERT_THAT(csfg_cpoly_eval(p, csfg_complex(5, 0)), ComplexEq(13, 0));
}

TEST_F(NAME, linear_imag)
{
    /* 3 + 2*s */
    csfg_cpoly_push(&p, csfg_complex(3, 0));
    csfg_cpoly_push(&p, csfg_complex(2, 0));
    ASSERT_THAT(csfg_cpoly_eval(p, csfg_complex(0, 0)), ComplexEq(3, 0));
    ASSERT_THAT(csfg_cpoly_eval(p, csfg_complex(0, 1)), ComplexEq(3, 2));
    ASSERT_THAT(csfg_cpoly_eval(p, csfg_complex(0, 5)), ComplexEq(3, 10));
}

TEST_F(NAME, linear_complex)
{
    /* 3 + 2*s */
    csfg_cpoly_push(&p, csfg_complex(3, 0));
    csfg_cpoly_push(&p, csfg_complex(2, 0));
    ASSERT_THAT(csfg_cpoly_eval(p, csfg_complex(0, 0)), ComplexEq(3, 0));
    ASSERT_THAT(csfg_cpoly_eval(p, csfg_complex(1, 1)), ComplexEq(5, 2));
    ASSERT_THAT(csfg_cpoly_eval(p, csfg_complex(5, 5)), ComplexEq(13, 10));
}
