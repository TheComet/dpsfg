#include "csfg/tests/Printers.hpp"

#include "gmock/gmock.h"

extern "C" {
#include "csfg/numeric/poly.h"
}

#define NAME test_cpoly_find_roots

MATCHER_P3(ComplexEq, real, imag, epsilon, "")
{
    return abs(arg.real - real) < epsilon && abs(arg.imag - imag) < epsilon;
}

using namespace testing;

struct NAME : public Test
{
    void SetUp() override
    {
        csfg_cpoly_init(&coeffs);
        csfg_rpoly_init(&roots);
    }
    void TearDown() override
    {
        csfg_rpoly_deinit(roots);
        csfg_cpoly_deinit(coeffs);
    }

    struct csfg_cpoly* coeffs;
    struct csfg_rpoly* roots;
    const double       epsilon = 1e-3;
};

TEST_F(NAME, single_value_has_no_roots)
{
    csfg_cpoly_push(&coeffs, csfg_complex(1.0, 0.0));

    csfg_cpoly_find_roots(&roots, coeffs, 0, 0.0);

    ASSERT_EQ(vec_count(roots), 0);
}

TEST_F(NAME, simple_root)
{
    // -1 + x
    csfg_cpoly_push(&coeffs, csfg_complex(-1.0, 0.0));
    csfg_cpoly_push(&coeffs, csfg_complex(1.0, 0.0));

    csfg_cpoly_find_roots(&roots, coeffs, 0, 0.0);

    ASSERT_EQ(vec_count(roots), 1);
    ASSERT_THAT(*vec_get(roots, 0), ComplexEq(1.0, 0.0, epsilon));
}

TEST_F(NAME, two_roots)
{
    // 1 + x - x^2
    csfg_cpoly_push(&coeffs, csfg_complex(1.0, 0.0));
    csfg_cpoly_push(&coeffs, csfg_complex(1.0, 0.0));
    csfg_cpoly_push(&coeffs, csfg_complex(-1.0, 0.0));

    csfg_cpoly_monic(coeffs);
    csfg_cpoly_find_roots(&roots, coeffs, 0, 0.0);

    ASSERT_EQ(vec_count(roots), 2);
    EXPECT_THAT(*vec_get(roots, 0), ComplexEq(1.6180340, 0.0, epsilon));
    EXPECT_THAT(*vec_get(roots, 1), ComplexEq(-0.618034, 0.0, epsilon));
}

TEST_F(NAME, two_repeated_roots)
{
    // (3x-4)^2 = (x - 4/3) = 16 - 24x + 9x^2
    csfg_cpoly_push(&coeffs, csfg_complex(16.0, 0.0));
    csfg_cpoly_push(&coeffs, csfg_complex(-24.0, 0.0));
    csfg_cpoly_push(&coeffs, csfg_complex(9.0, 0.0));

    csfg_cpoly_monic(coeffs);
    csfg_cpoly_find_roots(&roots, coeffs, 0, 0.0);

    ASSERT_EQ(vec_count(roots), 2);
    EXPECT_THAT(*vec_get(roots, 0), ComplexEq(4.0 / 3.0, 0.0, epsilon));
    EXPECT_THAT(*vec_get(roots, 1), ComplexEq(4.0 / 3.0, 0.0, epsilon));
}

TEST_F(NAME, three_repeated_roots)
{
    // (x-3)^3 = -27 + 27x - 9x^2 + x^3
    csfg_cpoly_push(&coeffs, csfg_complex(-27.0, 0.0));
    csfg_cpoly_push(&coeffs, csfg_complex(27.0, 0.0));
    csfg_cpoly_push(&coeffs, csfg_complex(-9.0, 0.0));
    csfg_cpoly_push(&coeffs, csfg_complex(1.0, 0.0));

    csfg_cpoly_monic(coeffs);
    csfg_cpoly_find_roots(&roots, coeffs, 0, 0.0);

    ASSERT_EQ(vec_count(roots), 3);
    EXPECT_THAT(*vec_get(roots, 0), ComplexEq(3.0, 0.0, epsilon));
    EXPECT_THAT(*vec_get(roots, 1), ComplexEq(3.0, 0.0, epsilon));
    EXPECT_THAT(*vec_get(roots, 2), ComplexEq(3.0, 0.0, epsilon));
}
