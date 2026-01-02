#include "gtest/gtest.h"

extern "C" {
#include "csfg/graph/graph.h"
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/var_table.h"
}

#define NAME test_graph_mason

using namespace testing;

struct NAME : public Test
{
    void SetUp() override
    {
        csfg_graph_init(&g);
        csfg_path_vec_init(&paths);
        csfg_path_vec_init(&loops);
        csfg_expr_pool_init(&pool);
        csfg_var_table_init(&vt);
    }
    void TearDown() override
    {
        csfg_var_table_deinit(&vt);
        csfg_expr_pool_deinit(pool);
        csfg_path_vec_deinit(loops);
        csfg_path_vec_deinit(paths);
        csfg_graph_deinit(&g);
    }

    struct csfg_graph      g;
    struct csfg_path_vec*  paths;
    struct csfg_path_vec*  loops;
    struct csfg_expr_pool* pool;
    struct csfg_var_table  vt;
};

TEST_F(NAME, two_nontouching_loops)
{
    int n1 = csfg_graph_add_node(&g, "n1");
    int n2 = csfg_graph_add_node(&g, "n2");
    int n3 = csfg_graph_add_node(&g, "n3");
    int n4 = csfg_graph_add_node(&g, "n4");
    /*
     *     ------<-----
     *    /      H2    \
     *  /               \
     * |  H1          H3 |
     * |  <-          <- |
     * |/   \       /   \|
     * o-->--o-->--o-->--o
     * n1 G1 n2 G2 n3 G3 n4
     */
    csfg_graph_add_edge_parse_expr(&g, n1, n2, cstr_view("G1"));
    csfg_graph_add_edge_parse_expr(&g, n2, n3, cstr_view("G2"));
    csfg_graph_add_edge_parse_expr(&g, n3, n4, cstr_view("G3"));
    csfg_graph_add_edge_parse_expr(&g, n4, n3, cstr_view("H3"));
    csfg_graph_add_edge_parse_expr(&g, n2, n1, cstr_view("H1"));
    csfg_graph_add_edge_parse_expr(&g, n4, n1, cstr_view("H2"));

    ASSERT_EQ(csfg_graph_find_forward_paths(&g, &paths, n1, n4), 0);
    ASSERT_EQ(csfg_graph_find_loops(&g, &loops), 0);
    int expr = csfg_graph_mason(&g, &pool, paths, loops);
    ASSERT_GE(expr, 0);

    // clang-format off
    double G1 = 3;  csfg_var_table_set_lit(&vt, cstr_view("G1"), G1);
    double G2 = 5;  csfg_var_table_set_lit(&vt, cstr_view("G2"), G2);
    double G3 = 7;  csfg_var_table_set_lit(&vt, cstr_view("G3"), G3);
    double H1 = 11; csfg_var_table_set_lit(&vt, cstr_view("H1"), H1);
    double H2 = 13; csfg_var_table_set_lit(&vt, cstr_view("H2"), H2);
    double H3 = 17; csfg_var_table_set_lit(&vt, cstr_view("H3"), H3);
    // obtained using SignalFlowGrapher
    ASSERT_DOUBLE_EQ(csfg_expr_eval(pool, expr, &vt), 
                       -G1*G2*G3/
        (G1*G2*G3*H2 - G1*G3*H1*H3 + G1*H1 + G3*H3 - 1)
    );
    // clang-format on
    /*
     *            G1*G2*G3
     * -------------------------------------
     * 1 - (G1*G2*G3*H2) - (G1*H1) - (G3*H3)
     */
}
TEST_F(NAME, andersen_ang_example2)
{
    int n1 = csfg_graph_add_node(&g, "V1");
    int n2 = csfg_graph_add_node(&g, "V2");
    int n3 = csfg_graph_add_node(&g, "V3");
    int n4 = csfg_graph_add_node(&g, "V4");
    int n5 = csfg_graph_add_node(&g, "V5");
    int n6 = csfg_graph_add_node(&g, "V5");
    int n7 = csfg_graph_add_node(&g, "V5");
    int n8 = csfg_graph_add_node(&g, "V5");
    /*
     *            H2      H3
     *          --<--   --<--
     *         /     \ /     \
     *     n5 o--->---o--->---o n7
     *       /   G2   n6  G3   \
     *   G1 ^                   v G4
     *     /                     \
     * n1 o                       o n8
     *     \                     /
     *   G5 v                   ^ G8
     *       \   G6   n3  G7   /
     *     n2 o--->---o--->---o n4
     *         \     / \     /
     *          --<--   --<--
     *            H6      H7
     */
    csfg_graph_add_edge_parse_expr(&g, n1, n5, cstr_view("G1"));
    csfg_graph_add_edge_parse_expr(&g, n5, n6, cstr_view("G2"));
    csfg_graph_add_edge_parse_expr(&g, n6, n7, cstr_view("G3"));
    csfg_graph_add_edge_parse_expr(&g, n7, n8, cstr_view("G4"));
    csfg_graph_add_edge_parse_expr(&g, n1, n2, cstr_view("G5"));
    csfg_graph_add_edge_parse_expr(&g, n2, n3, cstr_view("G6"));
    csfg_graph_add_edge_parse_expr(&g, n3, n4, cstr_view("G7"));
    csfg_graph_add_edge_parse_expr(&g, n4, n8, cstr_view("G8"));
    csfg_graph_add_edge_parse_expr(&g, n6, n5, cstr_view("H2"));
    csfg_graph_add_edge_parse_expr(&g, n7, n6, cstr_view("H3"));
    csfg_graph_add_edge_parse_expr(&g, n3, n2, cstr_view("H6"));
    csfg_graph_add_edge_parse_expr(&g, n4, n3, cstr_view("H7"));

    ASSERT_EQ(csfg_graph_find_forward_paths(&g, &paths, n1, n8), 0);
    ASSERT_EQ(csfg_graph_find_loops(&g, &loops), 0);
    int expr = csfg_graph_mason(&g, &pool, paths, loops);
    ASSERT_GE(expr, 0);

    // clang-format off
    double G1 = 3;  csfg_var_table_set_lit(&vt, cstr_view("G1"), G1);
    double G2 = 5;  csfg_var_table_set_lit(&vt, cstr_view("G2"), G2);
    double G3 = 7;  csfg_var_table_set_lit(&vt, cstr_view("G3"), G3);
    double G4 = 11; csfg_var_table_set_lit(&vt, cstr_view("G4"), G4);
    double G5 = 13; csfg_var_table_set_lit(&vt, cstr_view("G5"), G5);
    double G6 = 17; csfg_var_table_set_lit(&vt, cstr_view("G6"), G6);
    double G7 = 19; csfg_var_table_set_lit(&vt, cstr_view("G7"), G7);
    double G8 = 23; csfg_var_table_set_lit(&vt, cstr_view("G8"), G8);
    double H2 = 29; csfg_var_table_set_lit(&vt, cstr_view("H2"), H2);
    double H3 = 31; csfg_var_table_set_lit(&vt, cstr_view("H3"), H3);
    double H6 = 37; csfg_var_table_set_lit(&vt, cstr_view("H6"), H6);
    double H7 = 41; csfg_var_table_set_lit(&vt, cstr_view("H7"), H7);
    // obtained using SignalFlowGrapher
    ASSERT_DOUBLE_EQ(csfg_expr_eval(pool, expr, &vt), 
                (-G1*G2*G3*G4*(G6*H6 + G7*H7 - 1) - G5*G6*G7*G8*(G2*H2 + G3*H3 - 1)) /
        (G2*G6*H2*H6 + G2*G7*H2*H7 - G2*H2 + G3*G6*H3*H6 + G3*G7*H3*H7 - G3*H3 - G6*H6 - G7*H7 + 1));
    // original solution in pdf slides
    //ASSERT_DOUBLE_EQ(csfg_expr_eval(pool, expr, &vt), 
    //    (G5*G6*G7*G8*(1 - G2*H2 - G3*H3) + G1*G2*G3*G4*(1 - G6*H6 - G7*H7)) /
    //    (1 - (G2*H2 + G3*H3 + G6*H6 + G7*H7) + (G2*H2*G6*H6 + G2*H2*G7*H7 + G3*H3*G6*H6 + G3*H3*G7*H7))
    //);
    // clang-format on
}

TEST_F(NAME, active_lowpass_filter)
{
    int Vin = csfg_graph_add_node(&g, "Vin");
    int I2 = csfg_graph_add_node(&g, "I2");
    int V2 = csfg_graph_add_node(&g, "V2");
    int V3 = csfg_graph_add_node(&g, "V3");
    int V4 = csfg_graph_add_node(&g, "V4");
    int Vout = csfg_graph_add_node(&g, "Vout");
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
     *      G1    |    z2       '-. V4       Vout
     *            |                 o---->----o
     *            |             .-'      A    |
     *             \        o-'  1           /
     *              '-,     V3            ,-'
     *                  '-------<-------'
     *                       G2 + sC
     *
     * P1 = -G1*z2*A
     * L1 = -A*(G2+s*C)*z2
     */
    csfg_graph_add_edge_parse_expr(&g, Vin, I2, cstr_view("G1"));
    csfg_graph_add_edge_parse_expr(&g, I2, V2, cstr_view("z2"));
    csfg_graph_add_edge_parse_expr(&g, V2, V4, cstr_view("-1"));
    csfg_graph_add_edge_parse_expr(&g, V3, V4, cstr_view("1"));
    csfg_graph_add_edge_parse_expr(&g, V4, Vout, cstr_view("A"));
    csfg_graph_add_edge_parse_expr(&g, Vout, I2, cstr_view("G2+s*C"));

    ASSERT_EQ(csfg_graph_find_forward_paths(&g, &paths, Vin, Vout), 0);
    ASSERT_EQ(csfg_graph_find_loops(&g, &loops), 0);
    int expr = csfg_graph_mason(&g, &pool, paths, loops);
    ASSERT_GE(expr, 0);

    // clang-format off
    csfg_var_table_set_parse_expr(&vt, cstr_view("y2"), cstr_view("G1 + G2 + s*C"));
    csfg_var_table_set_parse_expr(&vt, cstr_view("z2"), cstr_view("1/y2"));
    double G1 = 3;  csfg_var_table_set_lit(&vt, cstr_view("G1"), G1);
    double G2 = 5;  csfg_var_table_set_lit(&vt, cstr_view("G2"), G2);
    double C  = 7;  csfg_var_table_set_lit(&vt, cstr_view("C"), C);
    double s  = 11; csfg_var_table_set_lit(&vt, cstr_view("s"), s);
    double A  = 13; csfg_var_table_set_lit(&vt, cstr_view("A"), A);
    double z2 = 1.0 / (G1 + G2 + s*C);
    ASSERT_DOUBLE_EQ(csfg_expr_eval(pool, expr, &vt), 
           (-G1*z2*A) /
        (1 + A*(G2+s*C)*z2)
    );
    // clang-format on
}
