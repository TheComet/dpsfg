#include "gmock/gmock.h"

extern "C" {
#include "csfg/numeric/poly.h"
}

#define NAME test_rpoly_partial_fraction_expansion

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

MATCHER_P3(
    PfdEq,
    expected_A,
    expected_p,
    expected_n,
    "matches a csfg_pfd approximately")
{
    return ExplainMatchResult(
        AllOf(
            Field(&csfg_pfd::A, expected_A),
            Field(&csfg_pfd::p, expected_p),
            Field(&csfg_pfd::n, Eq(1))),
        arg,
        result_listener);
}

} // namespace

struct NAME : public Test
{
    void SetUp() override
    {
        csfg_cpoly_init(&num);
        csfg_rpoly_init(&den);
        csfg_pfd_poly_init(&pfd);
    }
    void TearDown() override
    {
        csfg_pfd_poly_deinit(pfd);
        csfg_rpoly_deinit(den);
        csfg_cpoly_deinit(num);
    }

    struct csfg_cpoly*    num;
    struct csfg_rpoly*    den;
    struct csfg_pfd_poly* pfd;
};

TEST_F(NAME, example1)
{
    /*
     *     s-7
     * ----------
     * (s-1)(s+2)
     */
    csfg_cpoly_push(&num, csfg_complex(-7, 0));
    csfg_cpoly_push(&num, csfg_complex(1, 0));
    csfg_rpoly_push(&den, csfg_complex(1, 0));
    csfg_rpoly_push(&den, csfg_complex(-2, 0));

    ASSERT_THAT(
        csfg_rpoly_partial_fraction_decomposition(&pfd, num, den), Eq(0));

    /*
     * -2     3
     * --- + ---
     * s-1   s+2
     */
    ASSERT_THAT(vec_count(pfd), Eq(2));
    ASSERT_THAT(
        vec_get(pfd, 0),
        Pointee(AllOf(
            Field(&csfg_pfd::A, ComplexEq(-2, 0)),
            Field(&csfg_pfd::p, ComplexEq(1, 0)),
            Field(&csfg_pfd::n, Eq(1)))));
    ASSERT_THAT(
        vec_get(pfd, 1),
        Pointee(AllOf(
            Field(&csfg_pfd::A, ComplexEq(3, 0)),
            Field(&csfg_pfd::p, ComplexEq(-2, 0)),
            Field(&csfg_pfd::n, Eq(1)))));
}

TEST_F(NAME, example2)
{
    /*
     *     1
     * ----------
     * s(s-1)(s+1)
     */
    csfg_cpoly_push(&num, csfg_complex(1, 0));
    csfg_rpoly_push(&den, csfg_complex(0, 0));
    csfg_rpoly_push(&den, csfg_complex(1, 0));
    csfg_rpoly_push(&den, csfg_complex(-1, 0));

    ASSERT_THAT(
        csfg_rpoly_partial_fraction_decomposition(&pfd, num, den), Eq(0));

    /*
     * -1   1/2   1/2
     * -- + --- + ---
     *  s   s-1   s+1
     */
    ASSERT_THAT(vec_count(pfd), Eq(3));
    ASSERT_THAT(
        vec_get(pfd, 0),
        Pointee(AllOf(
            Field(&csfg_pfd::A, ComplexEq(-1, 0)),
            Field(&csfg_pfd::p, ComplexEq(0, 0)),
            Field(&csfg_pfd::n, Eq(1)))));
    ASSERT_THAT(
        vec_get(pfd, 1),
        Pointee(AllOf(
            Field(&csfg_pfd::A, ComplexEq(0.5, 0)),
            Field(&csfg_pfd::p, ComplexEq(1, 0)),
            Field(&csfg_pfd::n, Eq(1)))));
    ASSERT_THAT(
        vec_get(pfd, 2),
        Pointee(AllOf(
            Field(&csfg_pfd::A, ComplexEq(0.5, 0)),
            Field(&csfg_pfd::p, ComplexEq(-1, 0)),
            Field(&csfg_pfd::n, Eq(1)))));
}

TEST_F(NAME, example3)
{
    /*
     *      1
     * ------------
     * (s-1)^2(s+2)
     */
    csfg_cpoly_push(&num, csfg_complex(1, 0));
    csfg_rpoly_push(&den, csfg_complex(1, 0));
    csfg_rpoly_push(&den, csfg_complex(1, 0));
    csfg_rpoly_push(&den, csfg_complex(-2, 0));

    ASSERT_THAT(
        csfg_rpoly_partial_fraction_decomposition(&pfd, num, den), Eq(0));

    /*
     *   1/3     1/9   -1/9 
     * ------- + --- + -----
     * (s-1)^2   s+2   (s-1)
     */
    ASSERT_THAT(vec_count(pfd), Eq(3));
    ASSERT_THAT(
        vec_get(pfd, 0),
        Pointee(AllOf(
            Field(&csfg_pfd::A, ComplexEq(1.0 / 3.0, 0)),
            Field(&csfg_pfd::p, ComplexEq(1, 0)),
            Field(&csfg_pfd::n, Eq(2)))));
    ASSERT_THAT(
        vec_get(pfd, 1),
        Pointee(AllOf(
            Field(&csfg_pfd::A, ComplexEq(1.0 / 9.0, 0)),
            Field(&csfg_pfd::p, ComplexEq(-2, 0)),
            Field(&csfg_pfd::n, Eq(1)))));
    ASSERT_THAT(
        vec_get(pfd, 2),
        Pointee(AllOf(
            Field(&csfg_pfd::A, ComplexEq(-1.0 / 9.0, 0)),
            Field(&csfg_pfd::p, ComplexEq(1, 0)),
            Field(&csfg_pfd::n, Eq(1)))));
}

TEST_F(NAME, example4)
{
    /*
     * 3.5s^4 + 2s^3 - 4s^2 - 2s + 12.5
     * --------------------------------
     *         (s-1)^4(s+2)
     */
    csfg_cpoly_push(&num, csfg_complex(12.5, 0));
    csfg_cpoly_push(&num, csfg_complex(-2, 0));
    csfg_cpoly_push(&num, csfg_complex(-4, 0));
    csfg_cpoly_push(&num, csfg_complex(2, 0));
    csfg_cpoly_push(&num, csfg_complex(3.5, 0));
    csfg_rpoly_push(&den, csfg_complex(1, 0));
    csfg_rpoly_push(&den, csfg_complex(1, 0));
    csfg_rpoly_push(&den, csfg_complex(1, 0));
    csfg_rpoly_push(&den, csfg_complex(1, 0));
    csfg_rpoly_push(&den, csfg_complex(-2, 0));

    ASSERT_THAT(
        csfg_rpoly_partial_fraction_decomposition(&pfd, num, den), Eq(0));

    /*
     *    4         2         7        3     1/2
     * ------- + ------- + ------- + ----- + ---
     * (s-1)^4   (s-1)^3   (s-1)^2   (s-1)   s+2
     */
    ASSERT_THAT(vec_count(pfd), Eq(5));
    ASSERT_THAT(
        vec_get(pfd, 0),
        Pointee(AllOf(
            Field(&csfg_pfd::A, ComplexEq(4, 0)),
            Field(&csfg_pfd::p, ComplexEq(1, 0)),
            Field(&csfg_pfd::n, Eq(4)))));
    ASSERT_THAT(
        vec_get(pfd, 1),
        Pointee(AllOf(
            Field(&csfg_pfd::A, ComplexEq(2, 0)),
            Field(&csfg_pfd::p, ComplexEq(1, 0)),
            Field(&csfg_pfd::n, Eq(3)))));
    ASSERT_THAT(
        vec_get(pfd, 2),
        Pointee(AllOf(
            Field(&csfg_pfd::A, ComplexEq(7, 0)),
            Field(&csfg_pfd::p, ComplexEq(1, 0)),
            Field(&csfg_pfd::n, Eq(2)))));
    ASSERT_THAT(
        vec_get(pfd, 3),
        Pointee(AllOf(
            Field(&csfg_pfd::A, ComplexEq(3, 0)),
            Field(&csfg_pfd::p, ComplexEq(1, 0)),
            Field(&csfg_pfd::n, Eq(1)))));
    ASSERT_THAT(
        vec_get(pfd, 4),
        Pointee(AllOf(
            Field(&csfg_pfd::A, ComplexEq(0.5, 0)),
            Field(&csfg_pfd::p, ComplexEq(-2, 0)),
            Field(&csfg_pfd::n, Eq(1)))));
}
