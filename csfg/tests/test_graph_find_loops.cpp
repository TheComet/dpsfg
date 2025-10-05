#include "gmock/gmock.h"

extern "C" {
#include "csfg/graph/graph.h"
}

#define NAME test_graph_find_loops

using namespace testing;

struct NAME : public Test
{
    void SetUp() override
    {
        csfg_graph_init(&g);
        csfg_path_vec_init(&loops);
    }
    void TearDown() override
    {
        csfg_path_vec_deinit(loops);
        csfg_graph_deinit(&g);
    }

    struct csfg_graph     g;
    struct csfg_path_vec* loops;
};

TEST_F(NAME, single_loop_to_self)
{
    int n1 = csfg_graph_add_node(&g, "V1");
    int e1 = csfg_graph_add_edge_parse_expr(&g, n1, n1, cstr_view("5"));

    ASSERT_THAT(csfg_graph_find_loops(&g, &loops), Eq(0));
    ASSERT_THAT(vec_count(loops), Eq(2));
    ASSERT_THAT(vec_get(loops, 0), Pointee(e1));
    ASSERT_THAT(vec_get(loops, 1), Pointee(-1));
}

TEST_F(NAME, single_loop_with_two_nodes)
{
    int n1 = csfg_graph_add_node(&g, "V1");
    int n2 = csfg_graph_add_node(&g, "V2");

    int e1 = csfg_graph_add_edge_parse_expr(&g, n1, n2, cstr_view("5"));
    int e2 = csfg_graph_add_edge_parse_expr(&g, n2, n1, cstr_view("11"));

    ASSERT_THAT(csfg_graph_find_loops(&g, &loops), Eq(0));
    ASSERT_THAT(vec_count(loops), Eq(3));
    ASSERT_THAT(vec_get(loops, 0), Pointee(e1));
    ASSERT_THAT(vec_get(loops, 1), Pointee(e2));
    ASSERT_THAT(vec_get(loops, 2), Pointee(-1));
}

TEST_F(NAME, multiple_overlapping_loops)
{
    int n1 = csfg_graph_add_node(&g, "V1");
    int n2 = csfg_graph_add_node(&g, "V2");
    int n3 = csfg_graph_add_node(&g, "V3");
    int n4 = csfg_graph_add_node(&g, "V4");
    int n5 = csfg_graph_add_node(&g, "V5");

    /*
     *             5
     *        _   o
     *  1  2 / | /  4
     *  o---o---o---o
     *      \   3  /
     *       -----
     */

    int e1 = csfg_graph_add_edge_parse_expr(&g, n1, n2, cstr_view("5"));
    int e2 = csfg_graph_add_edge_parse_expr(&g, n2, n3, cstr_view("11"));
    int e3 = csfg_graph_add_edge_parse_expr(&g, n4, n3, cstr_view("3"));
    int e4 = csfg_graph_add_edge_parse_expr(&g, n3, n2, cstr_view("11"));
    int e5 = csfg_graph_add_edge_parse_expr(&g, n2, n4, cstr_view("3"));
    int e6 = csfg_graph_add_edge_parse_expr(&g, n3, n5, cstr_view("3"));
    (void)e1, (void)e3, (void)e5, (void)e6;

    ASSERT_THAT(csfg_graph_find_loops(&g, &loops), Eq(0));
    ASSERT_THAT(vec_count(loops), Eq(7));
    ASSERT_THAT(vec_get(loops, 0), Pointee(e2));
    ASSERT_THAT(vec_get(loops, 1), Pointee(e4));
    ASSERT_THAT(vec_get(loops, 2), Pointee(-1));
    ASSERT_THAT(vec_get(loops, 3), Pointee(e5));
    ASSERT_THAT(vec_get(loops, 4), Pointee(e3));
    ASSERT_THAT(vec_get(loops, 5), Pointee(e4));
    ASSERT_THAT(vec_get(loops, 6), Pointee(-1));
}
