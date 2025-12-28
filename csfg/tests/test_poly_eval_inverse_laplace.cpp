#include "gtest/gtest.h"

extern "C" {
#include "csfg/numeric/poly.h"
}

#define NAME test_pfd_poly_eval_inverse_laplace

using namespace testing;

struct NAME : public Test
{
    void SetUp() override { csfg_pfd_poly_init(&pfd); }
    void TearDown() override { csfg_pfd_poly_deinit(pfd); }

    struct csfg_pfd_poly* pfd;
};

TEST_F(NAME, test)
{
    struct csfg_pfd* term = csfg_pfd_poly_emplace(&pfd);
    term->A = csfg_complex(2, 0);
    term->p = csfg_complex(-2, 0);
    term->n = 1;

    ASSERT_EQ(csfg_pfd_poly_eval_inverse_laplace(pfd, 0), 2);
    ASSERT_EQ(csfg_pfd_poly_eval_inverse_laplace(pfd, 1), 2 * exp(-2));
}

TEST_F(NAME, lp2_repeated_roots)
{
    /*    2
     * -------
     * (s+3)^5
     */
    struct csfg_pfd* term = csfg_pfd_poly_emplace(&pfd);
    term->A = csfg_complex(2, 0);
    term->p = csfg_complex(-3, 0);
    term->n = 5;

    /*
     *  2
     * -- * t^4 * e^(-3*t^4)
     * 24
     */
    ASSERT_DOUBLE_EQ(csfg_pfd_poly_eval_inverse_laplace(pfd, 0), 0);
    ASSERT_DOUBLE_EQ(
        csfg_pfd_poly_eval_inverse_laplace(pfd, 0.1),
        2 / 24.0 * pow(0.1, 4) * exp(-3 * pow(0.1, 4)));
}
