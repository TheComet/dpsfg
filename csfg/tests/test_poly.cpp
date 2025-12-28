#include "csfg/tests/PolyHelper.hpp"

#include "gtest/gtest.h"

extern "C" {
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/poly.h"
}

#define NAME test_poly

using namespace testing;

struct NAME : public Test, public PolyHelper
{
    void SetUp() override
    {
        csfg_poly_init(&out);
        csfg_poly_init(&p1);
        csfg_poly_init(&p2);
        csfg_expr_pool_init(&pool);
    }
    void TearDown() override
    {
        csfg_expr_pool_deinit(pool);
        csfg_poly_deinit(p2);
        csfg_poly_deinit(p1);
        csfg_poly_deinit(out);
    }

    struct csfg_poly*      out;
    struct csfg_poly*      p1;
    struct csfg_poly*      p2;
    struct csfg_expr_pool* pool;
};

TEST_F(NAME, copy)
{
    csfg_poly_push(&p1, csfg_coeff(2.0, -1));
    csfg_poly_push(&p1, csfg_coeff(4.0, csfg_expr_var(&pool, cstr_view("a"))));
    csfg_poly_push(&p1, csfg_coeff(6.0, -1));
    csfg_poly_push(&p1, csfg_coeff(9.0, csfg_expr_var(&pool, cstr_view("b"))));

    ASSERT_EQ(csfg_poly_copy(&p2, p1), 0);

    ASSERT_TRUE(CoeffEq(pool, p2, 0, 2.0));
    ASSERT_TRUE(CoeffEq(pool, p2, 1, 4.0, "a"));
    ASSERT_TRUE(CoeffEq(pool, p2, 2, 6.0));
    ASSERT_TRUE(CoeffEq(pool, p2, 3, 9.0, "b"));
}

TEST_F(NAME, add_same_sized)
{
    csfg_poly_push(&p1, csfg_coeff(2.0, -1));
    csfg_poly_push(&p1, csfg_coeff(4.0, csfg_expr_var(&pool, cstr_view("a"))));
    csfg_poly_push(&p2, csfg_coeff(6.0, csfg_expr_var(&pool, cstr_view("b"))));
    csfg_poly_push(&p2, csfg_coeff(9.0, -1));

    /* (2+4ax) + (6b+9x) = 2+6b + (4a+9)x */
    ASSERT_EQ(csfg_poly_add(&pool, &out, p2, p1), 0);

    ASSERT_EQ(vec_count(out), 2);
    ASSERT_TRUE(CoeffEq(pool, out, 0, 1.0, "6*b+2"));
    ASSERT_TRUE(CoeffEq(pool, out, 1, 1.0, "9+4*a"));
}

TEST_F(NAME, add_smaller_to_larger)
{
    csfg_poly_push(&p1, csfg_coeff(2.0, -1));
    csfg_poly_push(&p2, csfg_coeff(4.0, csfg_expr_var(&pool, cstr_view("a"))));
    csfg_poly_push(&p2, csfg_coeff(6.0, csfg_expr_var(&pool, cstr_view("b"))));
    csfg_poly_push(&p2, csfg_coeff(9.0, -1));

    /* 2 + (4a + 6bx + 9x^2) = 2+4a + 6bx + 9x^2 */
    ASSERT_EQ(csfg_poly_add(&pool, &out, p2, p1), 0);

    ASSERT_EQ(vec_count(out), 3);
    ASSERT_TRUE(CoeffEq(pool, out, 0, 1.0, "4*a+2"));
    ASSERT_TRUE(CoeffEq(pool, out, 1, 6.0, "b"));
    ASSERT_TRUE(CoeffEq(pool, out, 2, 9.0));
}

TEST_F(NAME, add_larger_to_smaller)
{
    csfg_poly_push(&p1, csfg_coeff(2.0, -1));
    csfg_poly_push(&p1, csfg_coeff(4.0, csfg_expr_var(&pool, cstr_view("a"))));
    csfg_poly_push(&p1, csfg_coeff(6.0, csfg_expr_var(&pool, cstr_view("b"))));
    csfg_poly_push(&p2, csfg_coeff(9.0, -1));

    /* (2 + 4ax + 6bx^2) + 9 = 11 + 4ax + 6bx^2 */
    ASSERT_EQ(csfg_poly_add(&pool, &out, p2, p1), 0);

    ASSERT_EQ(vec_count(out), 3);
    ASSERT_TRUE(CoeffEq(pool, out, 0, 11.0));
    ASSERT_TRUE(CoeffEq(pool, out, 1, 4.0, "a"));
    ASSERT_TRUE(CoeffEq(pool, out, 2, 6.0, "b"));
}

TEST_F(NAME, mul_simple)
{
    GTEST_SKIP() << "TODO";
}
