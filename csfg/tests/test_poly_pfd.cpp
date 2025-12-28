#include "csfg/tests/Printers.hpp"

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
            Field(&csfg_complex::real, DoubleNear(expected_real, 1e-4)),
            Field(&csfg_complex::imag, DoubleNear(expected_imag, 1e-4))),
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

TEST_F(NAME, constant_expr)
{
    csfg_cpoly_push(&num, csfg_complex(42, 0));

    ASSERT_EQ(csfg_rpoly_partial_fraction_decomposition(&pfd, num, den), -1);
}

TEST_F(NAME, one_pole)
{
    /*
     *  7
     * ---
     * s+2
     */
    csfg_cpoly_push(&num, csfg_complex(7, 0));
    csfg_rpoly_push(&den, csfg_complex(-2, 0));

    ASSERT_EQ(csfg_rpoly_partial_fraction_decomposition(&pfd, num, den), 0);

    /*
     *  7
     * ---
     * s+2
     */
    ASSERT_EQ(vec_count(pfd), 1);
    ASSERT_THAT(vec_get(pfd, 0), Pointee(Field(&csfg_pfd::A, ComplexEq(7, 0))));
    ASSERT_THAT(
        vec_get(pfd, 0), Pointee(Field(&csfg_pfd::p, ComplexEq(-2, 0))));
    ASSERT_THAT(vec_get(pfd, 0), Pointee(Field(&csfg_pfd::n, 1)));
}

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

    ASSERT_EQ(csfg_rpoly_partial_fraction_decomposition(&pfd, num, den), 0);

    /*
     *   A1    A2    A1*s + 2*A1 + A2*s - A2
     * = --- + --- = -----------------------
     *   s-1   s+2         (s-1)(s+2)
     *
     *   (A1+A2)s + (2*A1-A2)
     * = --------------------
     *       (s-1)(s+2)
     *
     * s^0: -7 = 2*A1 - A2
     * s^1:  1 = A1 + A2
     *
     *   [-7] = [2 -1][A1]
     *   [ 1]   [1  1][A2]
     */

    /*
     * -2     3
     * --- + ---
     * s-1   s+2
     */
    ASSERT_EQ(vec_count(pfd), 2);
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
     *     1         A1   A2    A3
     * ---------- = --- + --- + ---
     * s(s-1)(s+1)  s-0   s-1   s+1
     */
    csfg_cpoly_push(&num, csfg_complex(1, 0));
    csfg_rpoly_push(&den, csfg_complex(0, 0));
    csfg_rpoly_push(&den, csfg_complex(1, 0));
    csfg_rpoly_push(&den, csfg_complex(-1, 0));

    ASSERT_EQ(csfg_rpoly_partial_fraction_decomposition(&pfd, num, den), 0);

    /*
     * -1   1/2   1/2
     * -- + --- + ---
     *  s   s-1   s+1
     */
    ASSERT_EQ(vec_count(pfd), 3);
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

    ASSERT_EQ(csfg_rpoly_partial_fraction_decomposition(&pfd, num, den), 0);

    /*
     *   A1      A2    A3    A1(s+2) + A2(s^2+s-2) + A3(s^2-2s+1)
     * ------- + --- + --- = ------------------------------------
     * (s-1)^2   s-1   s+2               (s-1)^2(s+2)
     *
     * s^0: 1 = 2*A1 - 2*A2 + A3
     * s^1: 0 = A1 + A2 - 2*A3
     * s^2: 0 = A2 + A3
     *
     * [1]   [2 -2  1][A1]
     * [0] = [1  1 -2][A2]
     * [0]   [0  1  1][A3]
     */

    /*
     *   1/3     1/9   -1/9
     * ------- + --- + -----
     * (s-1)^2   s+2   (s-1)
     */
    ASSERT_EQ(vec_count(pfd), 3);
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

TEST_F(NAME, example5)
{
    /*
     *     1
     * ----------
     * (s+3)(s+1)
     */
    csfg_cpoly_push(&num, csfg_complex(1, 0));
    csfg_rpoly_push(&den, csfg_complex(-3, 0));
    csfg_rpoly_push(&den, csfg_complex(-1, 0));

    ASSERT_EQ(csfg_rpoly_partial_fraction_decomposition(&pfd, num, den), 0);

    /*
     * 1/2   -1/2
     * --- + ----
     * s+3    s+1
     */
    ASSERT_EQ(vec_count(pfd), 2);
    ASSERT_THAT(
        vec_get(pfd, 0),
        Pointee(AllOf(
            Field(&csfg_pfd::A, ComplexEq(-0.5, 0)),
            Field(&csfg_pfd::p, ComplexEq(-3, 0)),
            Field(&csfg_pfd::n, Eq(1)))));
    ASSERT_THAT(
        vec_get(pfd, 1),
        Pointee(AllOf(
            Field(&csfg_pfd::A, ComplexEq(0.5, 0)),
            Field(&csfg_pfd::p, ComplexEq(-1, 0)),
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

    ASSERT_EQ(csfg_rpoly_partial_fraction_decomposition(&pfd, num, den), 0);

    /*
     *    4         2         7        3     1/2
     * ------- + ------- + ------- + ----- + ---
     * (s-1)^4   (s-1)^3   (s-1)^2   (s-1)   s+2
     */
    ASSERT_EQ(vec_count(pfd), 5);
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

TEST_F(NAME, lp2_repeated_roots)
{
    /*
     *       k*wp^2                  k=1
     * --------------------   with   wp=1
     * s^2 + s*wp/qp + wp^2          qp=0.5
     *
     *        1           A1        A2      A1 + A2*s + A2
     * -------------- = ------- + ----- = ----------------
     *  s^2 + 2*s + 1   (s+1)^2   (s+1)       (s+1)^2
     *
     *  s^1: 0 = A2
     *  s^0: 1 = A1 + A2
     */
    struct csfg_cpoly* den_coeffs;
    csfg_cpoly_init(&den_coeffs);
    csfg_cpoly_push(&num, csfg_complex(1, 0));
    csfg_cpoly_push(&den_coeffs, csfg_complex(1, 0));
    csfg_cpoly_push(&den_coeffs, csfg_complex(1 / 0.5, 0));
    csfg_cpoly_push(&den_coeffs, csfg_complex(1, 0));
    csfg_cpoly_find_roots(&den, den_coeffs, 100, 1e-6);
    csfg_cpoly_deinit(den_coeffs);

    ASSERT_EQ(csfg_rpoly_partial_fraction_decomposition(&pfd, num, den), 0);

    /*    1
     * -------
     * (s+1)^2
     */
    ASSERT_EQ(vec_count(pfd), 1);
    ASSERT_THAT(
        vec_get(pfd, 0),
        Pointee(AllOf(
            Field(&csfg_pfd::A, ComplexEq(1, 0)),
            Field(&csfg_pfd::p, ComplexEq(-1, 0)),
            Field(&csfg_pfd::n, Eq(2)))));
}
