#include "gmock/gmock.h"

extern "C" {
#include "csfg/graph/graph.h"
}

#define NAME test_graph_find_nontouching

using namespace testing;

struct NAME : public Test
{
    void SetUp() override
    {
        csfg_graph_init(&g);
        csfg_path_vec_init(&paths);
        csfg_path_vec_init(&loops);
        csfg_path_vec_init(&nontouching);
    }
    void TearDown() override
    {
        csfg_path_vec_deinit(nontouching);
        csfg_path_vec_deinit(loops);
        csfg_path_vec_deinit(paths);
        csfg_graph_deinit(&g);
    }

    struct csfg_graph     g;
    struct csfg_path_vec* paths;
    struct csfg_path_vec* loops;
    struct csfg_path_vec* nontouching;
};

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
    int G1 = csfg_graph_add_edge_parse_expr(&g, n1, n5, cstr_view("G1"));
    int G2 = csfg_graph_add_edge_parse_expr(&g, n5, n6, cstr_view("G2"));
    int G3 = csfg_graph_add_edge_parse_expr(&g, n6, n7, cstr_view("G3"));
    int G4 = csfg_graph_add_edge_parse_expr(&g, n7, n8, cstr_view("G4"));
    int G5 = csfg_graph_add_edge_parse_expr(&g, n1, n2, cstr_view("G5"));
    int G6 = csfg_graph_add_edge_parse_expr(&g, n2, n3, cstr_view("G6"));
    int G7 = csfg_graph_add_edge_parse_expr(&g, n3, n4, cstr_view("G7"));
    int G8 = csfg_graph_add_edge_parse_expr(&g, n4, n8, cstr_view("G8"));
    int H2 = csfg_graph_add_edge_parse_expr(&g, n6, n5, cstr_view("H2"));
    int H3 = csfg_graph_add_edge_parse_expr(&g, n7, n6, cstr_view("H3"));
    int H6 = csfg_graph_add_edge_parse_expr(&g, n3, n2, cstr_view("H6"));
    int H7 = csfg_graph_add_edge_parse_expr(&g, n4, n3, cstr_view("H7"));
    (void)G1, (void)G4, (void)G5, (void)G8;

    ASSERT_EQ(csfg_graph_find_forward_paths(&g, &paths, n1, n8), 0);
    ASSERT_EQ(csfg_graph_find_loops(&g, &loops), 0);
    ASSERT_EQ(vec_count(paths), 10);
    ASSERT_EQ(vec_count(loops), 12);

    csfg_path path;
    path.edge_idxs = vec_get(paths, 0); // G1*G2*G3*G4

    ASSERT_THAT(
        csfg_graph_find_nontouching(&g, &nontouching, paths, path), Eq(0));
    ASSERT_EQ(vec_count(nontouching), 0);

    ASSERT_THAT(
        csfg_graph_find_nontouching(&g, &nontouching, loops, path), Eq(0));
    ASSERT_EQ(vec_count(nontouching), 6);
    ASSERT_THAT(vec_get(nontouching, 0), Pointee(G6));
    ASSERT_THAT(vec_get(nontouching, 1), Pointee(H6));
    ASSERT_THAT(vec_get(nontouching, 2), Pointee(-1));
    ASSERT_THAT(vec_get(nontouching, 3), Pointee(G7));
    ASSERT_THAT(vec_get(nontouching, 4), Pointee(H7));
    ASSERT_THAT(vec_get(nontouching, 5), Pointee(-1));

    path.edge_idxs = vec_get(paths, 5); // G5*G6*G7*G8
    csfg_path_vec_clear(nontouching);

    ASSERT_EQ(csfg_graph_find_nontouching(&g, &nontouching, paths, path), 0);
    ASSERT_EQ(vec_count(nontouching), 0);

    ASSERT_EQ(csfg_graph_find_nontouching(&g, &nontouching, loops, path), 0);
    ASSERT_EQ(vec_count(nontouching), 6);
    ASSERT_THAT(vec_get(nontouching, 0), Pointee(G2));
    ASSERT_THAT(vec_get(nontouching, 1), Pointee(H2));
    ASSERT_THAT(vec_get(nontouching, 2), Pointee(-1));
    ASSERT_THAT(vec_get(nontouching, 3), Pointee(G3));
    ASSERT_THAT(vec_get(nontouching, 4), Pointee(H3));
    ASSERT_THAT(vec_get(nontouching, 5), Pointee(-1));
}
