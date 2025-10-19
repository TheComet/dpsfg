#include "gmock/gmock.h"

extern "C" {
#include "csfg/graph/graph.h"
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/expr_op.h"
#include "csfg/symbolic/expr_opt.h"
#include "csfg/symbolic/expr_tf.h"
#include "csfg/symbolic/rational.h"
#include "csfg/symbolic/var_table.h"
}

#define NAME test_expr_op_tf

using namespace testing;

struct NAME : public Test
{
    void SetUp() override
    {
        csfg_graph_init(&g);
        csfg_path_vec_init(&paths);
        csfg_path_vec_init(&loops);

        csfg_expr_pool_init(&pool1);
        csfg_expr_pool_init(&pool2);
        csfg_expr_pool_init(&pool3);
        csfg_var_table_init(&vt);
        csfg_rational_init(&rational);
    }
    void TearDown() override
    {
        csfg_rational_deinit(&rational);
        csfg_var_table_deinit(&vt);
        csfg_expr_pool_deinit(pool3);
        csfg_expr_pool_deinit(pool2);
        csfg_expr_pool_deinit(pool1);

        csfg_path_vec_deinit(loops);
        csfg_path_vec_deinit(paths);
        csfg_graph_deinit(&g);
    }

    struct csfg_graph     g;
    struct csfg_path_vec* paths;
    struct csfg_path_vec* loops;

    struct csfg_expr_pool* pool1;
    struct csfg_expr_pool* pool2;
    struct csfg_expr_pool* pool3;
    struct csfg_var_table  vt;
    struct csfg_rational   rational;
};

TEST_F(NAME, simple)
{
    /*
     *       1
     *   ---------         a * s
     *    1     1   --->   -----
     *   --- - ---         a - s
     *    s     a
     */
    int expr = csfg_expr_parse(&pool1, cstr_view("1/(1/s - 1/a)"));
    ASSERT_THAT(expr, Ge(0));
    ASSERT_THAT(
        csfg_expr_to_rational(pool1, expr, cstr_view("s"), &pool2, &rational),
        Eq(0));
}

TEST_F(NAME, light)
{
    /*
     *       1
     *   ---------         (s-1)*(a+4)        as + 4s - a - 4
     *    1     1   --->   -----------  --->  ---------------
     *   --- - ---          a+4 - s+1            a - s + 5
     *   s-1   a+4
     */
    int expr = csfg_expr_parse(&pool1, cstr_view("1/(1/(s-1)-1/(a+4))"));
    ASSERT_THAT(expr, Ge(0));
    csfg_expr_opt_remove_useless_ops(&pool1);
    csfg_expr_opt_fold_constants(&pool1);
    expr = csfg_expr_gc(pool1, expr);
    ASSERT_THAT(
        csfg_expr_to_rational(pool1, expr, cstr_view("s"), &pool2, &rational),
        Eq(0));
}

TEST_F(NAME, medium)
{
    /*
     *       1
     *   -------------        (s-1)^2*(a+4)        (s^2-2s+1)(a+4)
     *    1         1   --->  -------------  --->  ----------------
     *   ------- - ---        a+4 - (s-1)^2        a+4 - (s^2-2s+1)
     *   (s-1)^2   a+4
     *
     *       as^2 - 2as + a + 4s^2 -8s + 4        (a+4)s^2 - (2a+8)s + (a+4)
     * --->  -----------------------------  --->  --------------------------
     *            s^2 + 2s + a + 3                    s^2 + 2s + a + 3
     */
    int expr = csfg_expr_parse(&pool1, cstr_view("1/(1/(s-1)^2-1/(a+4))"));
    ASSERT_THAT(expr, Ge(0));
    csfg_expr_opt_remove_useless_ops(&pool1);
    csfg_expr_opt_fold_constants(&pool1);
    expr = csfg_expr_gc(pool1, expr);
    ASSERT_THAT(
        csfg_expr_to_rational(pool1, expr, cstr_view("s"), &pool2, &rational),
        Eq(0));
}

TEST_F(NAME, harder)
{
    int expr =
        csfg_expr_parse(&pool1, cstr_view("1/(1/s^3 - 4/(1+s)^2 - 8/(a+4))"));

    /*
     *              1                            1
     *     -------------------       -----------------------
     *      1       4       8  --->   1    4(a+4) - 8(1+s)^2
     *     --- - ------- - ---       --- - -----------------
     *     s^3   (1+s)^2   a+4       s^3     (a+4)(1+s)^2
     *
     *                   s^3(1+s)^2(a+4)
     * --->  --------------------------------------
     *       (a+4)(1+s)^2 - s^3(4(a+4) - 8(1+s)^2)
     *
     *                     s^3(s^2+2s+1)(a+4)
     * --->  --------------------------------------------
     *       (a+4)(s^2+2s+1) - 4s^3(a+4) - 8s^3(s^2+2s+1)
     *
     *                  as^5 + 2as^4 + as^3 + 4s^5 + 8s^4 + 4s^3
     * --->  ------------------------------------------------------------------
     *      as^2 + 2as + a + 4s^2 + 8s + 4 - 4as^3 - 16s^3 - 8s^5 - 16s^4 - 8s^3
     *
     *                  (a+4)s^5 + (2a+8)s^4 + (a+4)s^3
     * --->  -------------------------------------------------------------
     *       (-8)s^5 + (-16)s^4 + (-4a-24)s^3 + (a+4)s^2 + (2a+8)s + (a+4)
     */
    csfg_expr_opt_remove_useless_ops(&pool1);
    csfg_expr_opt_fold_constants(&pool1);
    expr = csfg_expr_gc(pool1, expr);
    ASSERT_THAT(
        csfg_expr_to_rational(pool1, expr, cstr_view("s"), &pool2, &rational),
        Eq(0));

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

TEST_F(NAME, compute_transfer_function)
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

TEST_F(NAME, inverting_amplifier)
{
    /*
     *                    G2
     *             +-----\/\/\----+
     *             |              |
     * Vin   G1    |  |'-.        |
     * O---\/\/\---o--| -  '-.    |     Vout
     *                |        >--o------O
     *             +--| +  .-'
     *            _|_ |.-'
     *                                               1
     *                                              --- = y2 = G1 + G2
     * Vin        I2        V2                       z2
     *  o---->----o---->----o-.  -1
     *      G1    |    z2       '-.          Vout
     *            |                 o---->----o
     *            |             .-'      A    |
     *             \        o-'  1           /
     *              '-,                   ,-'
     *                  '-------<-------'
     *                          G2
     *
     * P1 = -G1*z2*A
     * L1 = -A*G2*z2
     *
     *  -G1^2 - G1*G2   -G1 (G1+G2)   -G1
     *  ------------- = ----------- = ---
     *  G1*G2 + G2^2    G2 (G1+G2)     G2
     */
    int Vin = csfg_graph_add_node(&g, "Vin");
    int I2 = csfg_graph_add_node(&g, "I2");
    int V2 = csfg_graph_add_node(&g, "V2");
    int V3 = csfg_graph_add_node(&g, "V3");
    int V4 = csfg_graph_add_node(&g, "V4");
    int Vout = csfg_graph_add_node(&g, "Vout");

    csfg_graph_add_edge_parse_expr(&g, Vin, I2, cstr_view("G1"));
    csfg_graph_add_edge_parse_expr(&g, I2, V2, cstr_view("z2"));
    csfg_graph_add_edge_parse_expr(&g, V2, V3, cstr_view("-1"));
    csfg_graph_add_edge_parse_expr(&g, V4, V3, cstr_view("1"));
    csfg_graph_add_edge_parse_expr(&g, V3, Vout, cstr_view("A"));
    csfg_graph_add_edge_parse_expr(&g, Vout, I2, cstr_view("G2"));

    int expr;
    ASSERT_THAT(csfg_graph_find_forward_paths(&g, &paths, Vin, Vout), Eq(0));
    ASSERT_THAT(csfg_graph_find_loops(&g, &loops), Eq(0));
    expr = csfg_graph_mason(&g, &pool1, paths, loops);
    ASSERT_THAT(expr, Ge(0));

    csfg_var_table_set_parse_expr(&vt, cstr_view("z2"), cstr_view("1/y2"));
    csfg_var_table_set_parse_expr(&vt, cstr_view("y2"), cstr_view("G2"));
    csfg_var_table_set_parse_expr(&vt, cstr_view("A"), cstr_view("oo"));
    ASSERT_THAT(csfg_expr_insert_substitutions(&pool1, expr, &vt), Eq(0));
    csfg_expr_opt_remove_useless_ops(&pool1);
    csfg_expr_opt_fold_constants(&pool1);
    expr = csfg_expr_gc(pool1, expr);

    /*
     *    -G1*G2*A      -G1*G2*A          -G1*G2   -G1
     * -------------- = --------------- = ------ = ---
     * G2*(G2+(A*G2))   G2*G2 + G2*G2*A   G2*G2    G2
     */
    expr = csfg_expr_apply_limits(pool1, expr, &vt, &pool2);
    ASSERT_THAT(expr, Ge(0));
    ASSERT_THAT(
        csfg_expr_to_rational(pool2, expr, cstr_view("s"), &pool3, &rational),
        Eq(0));
}

TEST_F(NAME, integrator)
{
    /*
     *                    C
     *             +------||------+
     *             |              |
     * Vin   G1    |  |'-.        |
     * O---\/\/\---o--| -  '-.    |     Vout
     *                |        >--o------O
     *             +--| +  .-'
     *            _|_ |.-'
     *                                               1
     *                                              --- = y2 = G1 + sC
     * Vin        I2        V2                       z2
     *  o---->----o---->----o-.  -1
     *      G1    |    z2       '-. V3       Vout
     *            |                 o---->----o
     *            |             .-'      A    |
     *             \        o-'  1           /
     *              '-,     V4            ,-'
     *                  '-------<-------'
     *                         s*C
     *
     * P1 = -G1*z2*A
     * L1 = -A*(G2+s*C)*z2
     */
    int Vin = csfg_graph_add_node(&g, "Vin");
    int I2 = csfg_graph_add_node(&g, "I2");
    int V2 = csfg_graph_add_node(&g, "V2");
    int V3 = csfg_graph_add_node(&g, "V3");
    int V4 = csfg_graph_add_node(&g, "V4");
    int Vout = csfg_graph_add_node(&g, "Vout");

    csfg_graph_add_edge_parse_expr(&g, Vin, I2, cstr_view("G1"));
    csfg_graph_add_edge_parse_expr(&g, I2, V2, cstr_view("z2"));
    csfg_graph_add_edge_parse_expr(&g, V2, V3, cstr_view("-1"));
    csfg_graph_add_edge_parse_expr(&g, V4, V3, cstr_view("1"));
    csfg_graph_add_edge_parse_expr(&g, V3, Vout, cstr_view("A"));
    csfg_graph_add_edge_parse_expr(&g, Vout, I2, cstr_view("s*C"));

    int expr;
    ASSERT_THAT(csfg_graph_find_forward_paths(&g, &paths, Vin, Vout), Eq(0));
    ASSERT_THAT(csfg_graph_find_loops(&g, &loops), Eq(0));
    expr = csfg_graph_mason(&g, &pool1, paths, loops);
    ASSERT_THAT(expr, Ge(0));

    csfg_var_table_set_parse_expr(&vt, cstr_view("z2"), cstr_view("1/y2"));
    csfg_var_table_set_parse_expr(&vt, cstr_view("y2"), cstr_view("G2 + s*C"));
    csfg_var_table_set_parse_expr(&vt, cstr_view("A"), cstr_view("oo"));
    ASSERT_THAT(csfg_expr_insert_substitutions(&pool1, expr, &vt), Eq(0));
    csfg_expr_opt_remove_useless_ops(&pool1);
    csfg_expr_opt_fold_constants(&pool1);
    expr = csfg_expr_gc(pool1, expr);

    ASSERT_THAT(expr, Ge(0));
    /*
     *           -G1*(G2+s*C)*A           |        -G1*(G2+s*C)   -G1
     * ---------------------------------- |      = ------------ = ---
     * (G2+s*C)*(G2+s*C) + (G2+s*C)*s*C*A |A->oo   (G2+s*C)*s*C   s*C
     */
    expr = csfg_expr_apply_limits(pool1, expr, &vt, &pool2);

    /*
     *           -A*G1*G2 - A*G1*C*s
     * -----------------------------------------
     * G2^2 + 2*G2^2*C*(C+C*A)*s + C*(C+C*A)*s^2
     */
    ASSERT_THAT(
        csfg_expr_to_rational(pool2, expr, cstr_view("s"), &pool3, &rational),
        Eq(0));
}

TEST_F(NAME, active_lowpass_filter)
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
     *      G1    |    z2       '-. V3       Vout
     *            |                 o---->----o
     *            |             .-'      A    |
     *             \        o-'  1           /
     *              '-,     V4            ,-'
     *                  '-------<-------'
     *                       G2 + sC
     *
     * P1 = -G1*z2*A
     * L1 = -A*(G2+s*C)*z2
     *
     *
     *
     *       -G1*(s*C+G2+G1)              -G1
     * -------------------------------- = -----------
     * (s*C+G2+G1)*s*C + (s*C+G2+G1)*G2   G2 + s*C
     */
    int Vin = csfg_graph_add_node(&g, "Vin");
    int I2 = csfg_graph_add_node(&g, "I2");
    int V2 = csfg_graph_add_node(&g, "V2");
    int V3 = csfg_graph_add_node(&g, "V3");
    int V4 = csfg_graph_add_node(&g, "V4");
    int Vout = csfg_graph_add_node(&g, "Vout");

    csfg_graph_add_edge_parse_expr(&g, Vin, I2, cstr_view("G1"));
    csfg_graph_add_edge_parse_expr(&g, I2, V2, cstr_view("z2"));
    csfg_graph_add_edge_parse_expr(&g, V2, V3, cstr_view("-1"));
    csfg_graph_add_edge_parse_expr(&g, V4, V3, cstr_view("1"));
    csfg_graph_add_edge_parse_expr(&g, V3, Vout, cstr_view("A"));
    csfg_graph_add_edge_parse_expr(&g, Vout, I2, cstr_view("G2+s*C"));

    /*
     *   P1         -A*G1/(G1+G2+s*C)
     * ------ = --------------------------
     * 1 - L1   1 + A*(G2+s*C)/(G1+G2+s*C)
     */
    int expr;
    ASSERT_THAT(csfg_graph_find_forward_paths(&g, &paths, Vin, Vout), Eq(0));
    ASSERT_THAT(csfg_graph_find_loops(&g, &loops), Eq(0));
    expr = csfg_graph_mason(&g, &pool1, paths, loops);
    ASSERT_THAT(expr, Ge(0));

    csfg_var_table_set_parse_expr(&vt, cstr_view("z2"), cstr_view("1/y2"));
    csfg_var_table_set_parse_expr(&vt, cstr_view("y2"), cstr_view("s*C + G2"));
    csfg_var_table_set_parse_expr(&vt, cstr_view("A"), cstr_view("oo"));
    ASSERT_THAT(csfg_expr_insert_substitutions(&pool1, expr, &vt), Eq(0));
    csfg_expr_opt_remove_useless_ops(&pool1);
    csfg_expr_opt_fold_constants(&pool1);
    expr = csfg_expr_gc(pool1, expr);

    /*
     *             -A*G1             |          -G1
     * ----------------------------- |      = --------
     *   (G1+G2+s*C) - A*(G2+s*C)    |A->oo   G2 + s*C
     */
    expr = csfg_expr_apply_limits(pool1, expr, &vt, &pool2);
    /*
     * -G1*(G2+s*C)
     * -----------------
     * (G2+s*C)*(G2+s*C)
     */

    ASSERT_THAT(
        csfg_expr_to_rational(pool2, expr, cstr_view("s"), &pool3, &rational),
        Eq(0));
}
