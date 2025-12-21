#include "gmock/gmock.h"

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
