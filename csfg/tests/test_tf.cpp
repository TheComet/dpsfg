#include "gmock/gmock.h"

extern "C" {
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/var_table.h"
}

#define NAME test_tf

using namespace testing;

struct NAME : public Test
{
    void SetUp() override
    {
        csfg_expr_pool_init(&p);
        csfg_var_table_init(&vt);
        csfg_expr_vec_init(&num);
        csfg_expr_vec_init(&den);
    }
    void TearDown() override
    {
        csfg_expr_vec_deinit(den);
        csfg_expr_vec_deinit(num);
        csfg_var_table_deinit(&vt);
        csfg_expr_pool_deinit(p);
    }

    struct csfg_expr_pool* p;
    struct csfg_var_table  vt;
    struct csfg_expr_vec*  num;
    struct csfg_expr_vec*  den;
};

TEST_F(NAME, compute_transfer_function_coefficient_expressions)
{
    int e = csfg_expr_parse(&p, "1/(1/s^3 - 4/(1+s)^2 - 8/(a+4))");
    // int e = csfg_expr_parse(&p, "(s+a)^2 / (((s+b)^2 * s^2)*(a+b*s)*s)");
    ASSERT_THAT(e, Ge(0));

    /*
     *              1
     *     -------------------
     *      1       4       8
     *     --- - ------- - ---
     *     s^3   (1+s)^2   a+4
     *
     *     ( s^-3 - 4(1+s)^-2 - 8(a+4)^-1 )^-1
     *
     * Solved on paper:
     *                      s^3 + 2s^4 + s^5
     *  G(s) = ---------------------------------------------
     *         1 + 2s + s^2 + (-4-x)s^3 + (-2x)s^4 + (-x)s^5
     * where:
     *     x = 8/(a+4)
     *
     * b0 = 0
     * b1 = 0
     * b2 = 0
     * b3 = 1
     * b4 = 2
     * b5 = 1
     * a0 = 1
     * a1 = 2
     * a2 = 1
     * a3 = -4-8/(a+4)
     * a4 = -16/(a+4)
     * a5 = -8/(a+4)
     */
    int div = csfg_expr_to_standard_tf(&p, e, cstr_view("s"));
    ASSERT_THAT(div, Ge(0));

    csfg_expr_standard_tf_to_coeff(&p, div, cstr_view("s"), &num, &den);

#if 0
    for (std::size_t i = 0; i != tfc.numerator.size(); ++i)
    {
        std::stringstream ss;
        ss << "numerator, degree " << i;
        tfc.numerator[i]->dump(
            "compute_transfer_function_coefficient_expressions.dot",
            true,
            ss.str().c_str());
    }
    for (std::size_t i = 0; i != tfc.denominator.size(); ++i)
    {
        std::stringstream ss;
        ss << "denominator, degree " << i;
        tfc.denominator[i]->dump(
            "compute_transfer_function_coefficient_expressions.dot",
            true,
            ss.str().c_str());
    }

    Reference<VariableTable> vt = new VariableTable;
    vt->set("a", 7.2);
    ASSERT_THAT(tfc.numerator.size(), Eq(6u));
    ASSERT_THAT(tfc.denominator.size(), Eq(6u));

    ASSERT_THAT(tfc.numerator[0]->evaluate(vt), DoubleEq(0.0));
    ASSERT_THAT(tfc.numerator[1]->evaluate(vt), DoubleEq(0.0));
    ASSERT_THAT(tfc.numerator[2]->evaluate(vt), DoubleEq(0.0));
    ASSERT_THAT(tfc.numerator[3]->evaluate(vt), DoubleEq(1.0));
    ASSERT_THAT(tfc.numerator[4]->evaluate(vt), DoubleEq(2.0));
    ASSERT_THAT(tfc.numerator[5]->evaluate(vt), DoubleEq(1.0));

    ASSERT_THAT(tfc.denominator[0]->evaluate(vt), DoubleEq(1.0));
    ASSERT_THAT(tfc.denominator[1]->evaluate(vt), DoubleEq(2.0));
    ASSERT_THAT(tfc.denominator[2]->evaluate(vt), DoubleEq(1.0));
    ASSERT_THAT(
        tfc.denominator[3]->evaluate(vt), DoubleEq(-4.0 - 8.0 / (7.2 + 4.0)));
    ASSERT_THAT(
        tfc.denominator[4]->evaluate(vt), DoubleEq(-16.0 / (7.2 + 4.0)));
    ASSERT_THAT(tfc.denominator[5]->evaluate(vt), DoubleEq(-8.0 / (7.2 + 4.0)));
#endif
}

TEST(NAME, compute_transfer_function)
{
#if 0
    Reference<Expression> e =
        Expression::parse("1/(1/s^3 - 4/(1+s)^2 - 8/(a+4))");
    /*
     * Solved on paper:
     *                      s^3 + 2s^4 + s^5
     *  G(s) = ---------------------------------------------
     *         1 + 2s + s^2 + (-4-x)s^3 + (-2x)s^4 + (-x)s^5
     * where:
     *     x = 8/(a+4)
     *
     * b0 = 0
     * b1 = 0
     * b2 = 0
     * b3 = 1
     * b4 = 2
     * b5 = 1
     * a0 = 1
     * a1 = 2
     * a2 = 1
     * a3 = -4-8/(a+4)
     * a4 = -16/(a+4)
     * a5 = -8/(a+4)
     */
    e->dump("compute_transfer_function.dot");
    TFManipulator::TFCoefficients tfc =
        TFManipulator::calculateTransferFunctionCoefficients(e, "s");

    for (std::size_t i = 0; i != tfc.numerator.size(); ++i)
    {
        std::stringstream ss;
        ss << "numerator, degree " << i;
        tfc.numerator[i]->dump(
            "compute_transfer_function.dot", true, ss.str().c_str());
    }
    for (std::size_t i = 0; i != tfc.denominator.size(); ++i)
    {
        std::stringstream ss;
        ss << "denominator, degree " << i;
        tfc.denominator[i]->dump(
            "compute_transfer_function.dot", true, ss.str().c_str());
    }

    Reference<VariableTable> vt = new VariableTable;
    vt->set("a", 7.2);
    TransferFunction tf = TFManipulator::calculateTransferFunction(tfc, vt);
    ASSERT_THAT(tf.roots(), Eq(5));
    ASSERT_THAT(tf.poles(), Eq(5));
#endif
}

TEST(NAME, active_lowpass_filter)
{
    /*
     *                    C
     *             +------||------+
     *             |              |
     *             |      G2      |
     *             o-----\/\/\----o
     *             |              |
     * Vin   G1    |  |'-.        |
     * O---\/\/\---o--| -  '-.    |     Vout
     *                |        >--o------O
     *             +--| +  .-'
     *            _|_ |.-'
     *                                               1
     *                                              --- = y2 = G1 + G2 + sC
     * Vin        I2        V2                       z2
     *  o---->----o---->----o-.  -1
     *      G1    |    z2       '-.          Vout
     *            |                 o---->----o
     *            |             .-'      A    |
     *             \        o-'  1           /
     *              '-,                   ,-'
     *                  '-------<-------'
     *                       G2 + sC
     *
     * P1 = -G1*z2*A
     * L1 = -A*(G2+s*C)*z2
     */
#if 0
    Reference<Expression> e = Expression::parse("P1/(1-L1)");
    e->dump("active_lowpass_filter.dot");

    Reference<VariableTable> vt = new VariableTable;
    vt->set("P1", "-G1*z2*A");
    vt->set("L1", "-A*(G2+s*C)*z2");
    vt->set("z2", "1/(G1 + G2 + s*C)");
    e->insertSubstitutions(vt);
    e->dump("active_lowpass_filter.dot", true, "Substitution");

    TFManipulator::TFCoefficients tfc =
        TFManipulator::calculateTransferFunctionCoefficients(e, "s");
    for (std::size_t i = 0; i != tfc.numerator.size(); ++i)
    {
        std::stringstream ss;
        ss << "numerator, degree " << i;
        tfc.numerator[i]->dump(
            "compute_transfer_function.dot", true, ss.str().c_str());
    }
    for (std::size_t i = 0; i != tfc.denominator.size(); ++i)
    {
        std::stringstream ss;
        ss << "denominator, degree " << i;
        tfc.denominator[i]->dump(
            "compute_transfer_function.dot", true, ss.str().c_str());
    }

    TransferFunction tf = TFManipulator::calculateTransferFunction(tfc, vt);
    ASSERT_THAT(tf.roots(), Eq(5));
    ASSERT_THAT(tf.poles(), Eq(5));
#endif
}
