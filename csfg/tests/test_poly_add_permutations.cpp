#include "csfg/tests/PolyHelper.hpp"

#include "gmock/gmock.h"

extern "C" {
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/poly.h"
}

#define NAME test_poly_add_permutations

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

TEST_F(NAME, add_0_0)
{
    csfg_poly_push(&p1, csfg_coeff(0.0, -1));
    csfg_poly_push(&p2, csfg_coeff(0.0, -1));

    ASSERT_THAT(csfg_poly_add(&pool, &out, p1, p2), Eq(0));

    ASSERT_TRUE(CoeffEq(pool, out, 0, 0.0));
}

TEST_F(NAME, add_0a_0)
{
    csfg_poly_push(&p1, csfg_coeff(0.0, csfg_expr_var(&pool, cstr_view("a"))));
    csfg_poly_push(&p2, csfg_coeff(0.0, -1));

    ASSERT_THAT(csfg_poly_add(&pool, &out, p1, p2), Eq(0));

    ASSERT_TRUE(CoeffEq(pool, out, 0, 0.0));
}

TEST_F(NAME, add_0_0b)
{
    csfg_poly_push(&p1, csfg_coeff(0.0, -1));
    csfg_poly_push(&p2, csfg_coeff(0.0, csfg_expr_var(&pool, cstr_view("a"))));

    ASSERT_THAT(csfg_poly_add(&pool, &out, p1, p2), Eq(0));

    ASSERT_TRUE(CoeffEq(pool, out, 0, 0.0));
}

TEST_F(NAME, add_0a_0b)
{
    csfg_poly_push(&p1, csfg_coeff(0.0, csfg_expr_var(&pool, cstr_view("a"))));
    csfg_poly_push(&p2, csfg_coeff(0.0, csfg_expr_var(&pool, cstr_view("b"))));

    ASSERT_THAT(csfg_poly_add(&pool, &out, p1, p2), Eq(0));

    ASSERT_TRUE(CoeffEq(pool, out, 0, 0.0));
}

TEST_F(NAME, add_1_1)
{
    csfg_poly_push(&p1, csfg_coeff(1.0, -1));
    csfg_poly_push(&p2, csfg_coeff(1.0, -1));

    ASSERT_THAT(csfg_poly_add(&pool, &out, p1, p2), Eq(0));

    ASSERT_TRUE(CoeffEq(pool, out, 0, 2.0));
}

TEST_F(NAME, add_1a_1)
{
    csfg_poly_push(&p1, csfg_coeff(1.0, csfg_expr_var(&pool, cstr_view("a"))));
    csfg_poly_push(&p2, csfg_coeff(1.0, -1));

    ASSERT_THAT(csfg_poly_add(&pool, &out, p1, p2), Eq(0));

    ASSERT_TRUE(CoeffEq(pool, out, 0, 1.0, "a+1"));
}

TEST_F(NAME, add_1_1b)
{
    csfg_poly_push(&p1, csfg_coeff(1.0, -1));
    csfg_poly_push(&p2, csfg_coeff(1.0, csfg_expr_var(&pool, cstr_view("b"))));

    ASSERT_THAT(csfg_poly_add(&pool, &out, p1, p2), Eq(0));

    ASSERT_TRUE(CoeffEq(pool, out, 0, 1.0, "1+b"));
}

TEST_F(NAME, add_1a_1b)
{
    csfg_poly_push(&p1, csfg_coeff(1.0, csfg_expr_var(&pool, cstr_view("a"))));
    csfg_poly_push(&p2, csfg_coeff(1.0, csfg_expr_var(&pool, cstr_view("b"))));

    ASSERT_THAT(csfg_poly_add(&pool, &out, p1, p2), Eq(0));

    ASSERT_TRUE(CoeffEq(pool, out, 0, 1.0, "a+b"));
}

TEST_F(NAME, add_1_3)
{
    csfg_poly_push(&p1, csfg_coeff(1.0, -1));
    csfg_poly_push(&p2, csfg_coeff(3.0, -1));

    ASSERT_THAT(csfg_poly_add(&pool, &out, p1, p2), Eq(0));

    ASSERT_TRUE(CoeffEq(pool, out, 0, 4.0));
}

TEST_F(NAME, add_1a_3)
{
    csfg_poly_push(&p1, csfg_coeff(1.0, csfg_expr_var(&pool, cstr_view("a"))));
    csfg_poly_push(&p2, csfg_coeff(3.0, -1));

    ASSERT_THAT(csfg_poly_add(&pool, &out, p1, p2), Eq(0));

    ASSERT_TRUE(CoeffEq(pool, out, 0, 1.0, "a+3"));
}

TEST_F(NAME, add_1_3b)
{
    csfg_poly_push(&p1, csfg_coeff(1.0, -1));
    csfg_poly_push(&p2, csfg_coeff(3.0, csfg_expr_var(&pool, cstr_view("b"))));

    ASSERT_THAT(csfg_poly_add(&pool, &out, p1, p2), Eq(0));

    ASSERT_TRUE(CoeffEq(pool, out, 0, 1.0, "1+3*b"));
}

TEST_F(NAME, add_1a_3b)
{
    csfg_poly_push(&p1, csfg_coeff(1.0, csfg_expr_var(&pool, cstr_view("a"))));
    csfg_poly_push(&p2, csfg_coeff(3.0, csfg_expr_var(&pool, cstr_view("b"))));

    ASSERT_THAT(csfg_poly_add(&pool, &out, p1, p2), Eq(0));

    ASSERT_TRUE(CoeffEq(pool, out, 0, 1.0, "a+3*b"));
}

TEST_F(NAME, add_2_1)
{
    csfg_poly_push(&p1, csfg_coeff(2.0, -1));
    csfg_poly_push(&p2, csfg_coeff(1.0, -1));

    ASSERT_THAT(csfg_poly_add(&pool, &out, p1, p2), Eq(0));

    ASSERT_TRUE(CoeffEq(pool, out, 0, 3.0));
}

TEST_F(NAME, add_2a_1)
{
    csfg_poly_push(&p1, csfg_coeff(2.0, csfg_expr_var(&pool, cstr_view("a"))));
    csfg_poly_push(&p2, csfg_coeff(1.0, -1));

    ASSERT_THAT(csfg_poly_add(&pool, &out, p1, p2), Eq(0));

    ASSERT_TRUE(CoeffEq(pool, out, 0, 1.0, "2*a+1"));
}

TEST_F(NAME, add_2_1b)
{
    csfg_poly_push(&p1, csfg_coeff(2.0, -1));
    csfg_poly_push(&p2, csfg_coeff(1.0, csfg_expr_var(&pool, cstr_view("b"))));

    ASSERT_THAT(csfg_poly_add(&pool, &out, p1, p2), Eq(0));

    ASSERT_TRUE(CoeffEq(pool, out, 0, 1.0, "2+b"));
}

TEST_F(NAME, add_2a_1b)
{
    csfg_poly_push(&p1, csfg_coeff(2.0, csfg_expr_var(&pool, cstr_view("a"))));
    csfg_poly_push(&p2, csfg_coeff(1.0, csfg_expr_var(&pool, cstr_view("b"))));

    ASSERT_THAT(csfg_poly_add(&pool, &out, p1, p2), Eq(0));

    ASSERT_TRUE(CoeffEq(pool, out, 0, 1.0, "2*a+b"));
}

TEST_F(NAME, add_2_3)
{
    csfg_poly_push(&p1, csfg_coeff(2.0, -1));
    csfg_poly_push(&p2, csfg_coeff(3.0, -1));

    ASSERT_THAT(csfg_poly_add(&pool, &out, p1, p2), Eq(0));

    ASSERT_TRUE(CoeffEq(pool, out, 0, 5.0));
}

TEST_F(NAME, add_2a_3)
{
    csfg_poly_push(&p1, csfg_coeff(2.0, csfg_expr_var(&pool, cstr_view("a"))));
    csfg_poly_push(&p2, csfg_coeff(3.0, -1));

    ASSERT_THAT(csfg_poly_add(&pool, &out, p1, p2), Eq(0));

    ASSERT_TRUE(CoeffEq(pool, out, 0, 1.0, "2*a+3"));
}

TEST_F(NAME, add_2_3b)
{
    csfg_poly_push(&p1, csfg_coeff(2.0, -1));
    csfg_poly_push(&p2, csfg_coeff(3.0, csfg_expr_var(&pool, cstr_view("b"))));

    ASSERT_THAT(csfg_poly_add(&pool, &out, p1, p2), Eq(0));

    ASSERT_TRUE(CoeffEq(pool, out, 0, 1.0, "2+3*b"));
}

TEST_F(NAME, add_2a_3b)
{
    csfg_poly_push(&p1, csfg_coeff(2.0, csfg_expr_var(&pool, cstr_view("a"))));
    csfg_poly_push(&p2, csfg_coeff(3.0, csfg_expr_var(&pool, cstr_view("b"))));

    ASSERT_THAT(csfg_poly_add(&pool, &out, p1, p2), Eq(0));

    ASSERT_TRUE(CoeffEq(pool, out, 0, 1.0, "2*a+3*b"));
}
