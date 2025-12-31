#include "gmock/gmock.h"

extern "C" {
#include "csfg/graph/graph.h"
}

#define NAME test_graph_find_forward_paths

using namespace testing;

struct NAME : public Test
{
    void SetUp() override
    {
        csfg_graph_init(&g);
        csfg_path_vec_init(&paths);
    }
    void TearDown() override
    {
        csfg_path_vec_deinit(paths);
        csfg_graph_deinit(&g);
    }

    struct csfg_graph     g;
    struct csfg_path_vec* paths;
};

TEST_F(NAME, single_forward_path)
{
    int n1 = csfg_graph_add_node(&g, "V1");
    int n2 = csfg_graph_add_node(&g, "V2");
    int n3 = csfg_graph_add_node(&g, "V3");

    int e1 = csfg_graph_add_edge_parse_expr(&g, n1, n2, cstr_view("5"));
    int e2 = csfg_graph_add_edge_parse_expr(&g, n2, n3, cstr_view("11"));

    ASSERT_EQ(csfg_graph_find_forward_paths(&g, &paths, n1, n3), 0);
    ASSERT_EQ(vec_count(paths), 3);
    ASSERT_THAT(vec_get(paths, 0), Pointee(e1));
    ASSERT_THAT(vec_get(paths, 1), Pointee(e2));
    ASSERT_THAT(vec_get(paths, 2), Pointee(-1));
}

TEST_F(NAME, two_separate_forward_paths)
{
    int n1 = csfg_graph_add_node(&g, "V1");
    int n2 = csfg_graph_add_node(&g, "V2");
    int n3 = csfg_graph_add_node(&g, "V3");
    int n4 = csfg_graph_add_node(&g, "V4");

    int e1 = csfg_graph_add_edge_parse_expr(&g, n1, n2, cstr_view("5"));
    int e2 = csfg_graph_add_edge_parse_expr(&g, n2, n3, cstr_view("11"));

    int e3 = csfg_graph_add_edge_parse_expr(&g, n1, n4, cstr_view("3"));
    int e4 = csfg_graph_add_edge_parse_expr(&g, n4, n3, cstr_view("13"));

    ASSERT_EQ(csfg_graph_find_forward_paths(&g, &paths, n1, n3), 0);
    ASSERT_EQ(vec_count(paths), 6);
    ASSERT_THAT(vec_get(paths, 0), Pointee(e1));
    ASSERT_THAT(vec_get(paths, 1), Pointee(e2));
    ASSERT_THAT(vec_get(paths, 2), Pointee(-1));
    ASSERT_THAT(vec_get(paths, 3), Pointee(e3));
    ASSERT_THAT(vec_get(paths, 4), Pointee(e4));
    ASSERT_THAT(vec_get(paths, 5), Pointee(-1));
}

TEST_F(NAME, two_joined_forward_paths)
{
    int n1 = csfg_graph_add_node(&g, "V1");
    int n2 = csfg_graph_add_node(&g, "V2");
    int n3 = csfg_graph_add_node(&g, "V3");

    int e1 = csfg_graph_add_edge_parse_expr(&g, n1, n2, cstr_view("5"));
    int e2 = csfg_graph_add_edge_parse_expr(&g, n2, n3, cstr_view("11"));
    int e3 = csfg_graph_add_edge_parse_expr(&g, n2, n3, cstr_view("3"));

    ASSERT_EQ(csfg_graph_find_forward_paths(&g, &paths, n1, n3), 0);
    ASSERT_EQ(vec_count(paths), 6);
    ASSERT_THAT(vec_get(paths, 0), Pointee(e1));
    ASSERT_THAT(vec_get(paths, 1), Pointee(e2));
    ASSERT_THAT(vec_get(paths, 2), Pointee(-1));
    ASSERT_THAT(vec_get(paths, 3), Pointee(e1));
    ASSERT_THAT(vec_get(paths, 4), Pointee(e3));
    ASSERT_THAT(vec_get(paths, 5), Pointee(-1));
}

TEST_F(NAME, multiple_paths_with_dead_ends)
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
    int e3 = csfg_graph_add_edge_parse_expr(&g, n3, n4, cstr_view("3"));
    int e4 = csfg_graph_add_edge_parse_expr(&g, n2, n3, cstr_view("11"));
    int e5 = csfg_graph_add_edge_parse_expr(&g, n2, n4, cstr_view("3"));
    int e6 = csfg_graph_add_edge_parse_expr(&g, n3, n5, cstr_view("3"));

    ASSERT_EQ(csfg_graph_find_forward_paths(&g, &paths, n1, n4), 0);
    ASSERT_EQ(vec_count(paths), 11);
    ASSERT_THAT(vec_get(paths, 0), Pointee(e1));
    ASSERT_THAT(vec_get(paths, 1), Pointee(e2));
    ASSERT_THAT(vec_get(paths, 2), Pointee(e3));
    ASSERT_THAT(vec_get(paths, 3), Pointee(-1));
    ASSERT_THAT(vec_get(paths, 4), Pointee(e1));
    ASSERT_THAT(vec_get(paths, 5), Pointee(e4));
    ASSERT_THAT(vec_get(paths, 6), Pointee(e3));
    ASSERT_THAT(vec_get(paths, 7), Pointee(-1));
    ASSERT_THAT(vec_get(paths, 8), Pointee(e1));
    ASSERT_THAT(vec_get(paths, 9), Pointee(e5));
    ASSERT_THAT(vec_get(paths, 10), Pointee(-1));

    ASSERT_EQ(csfg_graph_find_forward_paths(&g, &paths, n1, n5), 0);
    ASSERT_EQ(vec_count(paths), 8);
    ASSERT_THAT(vec_get(paths, 0), Pointee(e1));
    ASSERT_THAT(vec_get(paths, 1), Pointee(e2));
    ASSERT_THAT(vec_get(paths, 2), Pointee(e6));
    ASSERT_THAT(vec_get(paths, 3), Pointee(-1));
    ASSERT_THAT(vec_get(paths, 4), Pointee(e1));
    ASSERT_THAT(vec_get(paths, 5), Pointee(e4));
    ASSERT_THAT(vec_get(paths, 6), Pointee(e6));
    ASSERT_THAT(vec_get(paths, 7), Pointee(-1));
}

TEST_F(NAME, multiple_paths_with_loops_and_dead_ends)
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
    int e3 = csfg_graph_add_edge_parse_expr(&g, n3, n4, cstr_view("3"));
    int e4 = csfg_graph_add_edge_parse_expr(&g, n3, n2, cstr_view("11"));
    int e5 = csfg_graph_add_edge_parse_expr(&g, n2, n4, cstr_view("3"));
    int e6 = csfg_graph_add_edge_parse_expr(&g, n3, n5, cstr_view("3"));
    (void)e4;

    ASSERT_EQ(csfg_graph_find_forward_paths(&g, &paths, n1, n4), 0);
    ASSERT_EQ(vec_count(paths), 7);
    ASSERT_THAT(vec_get(paths, 0), Pointee(e1));
    ASSERT_THAT(vec_get(paths, 1), Pointee(e2));
    ASSERT_THAT(vec_get(paths, 2), Pointee(e3));
    ASSERT_THAT(vec_get(paths, 3), Pointee(-1));
    ASSERT_THAT(vec_get(paths, 4), Pointee(e1));
    ASSERT_THAT(vec_get(paths, 5), Pointee(e5));
    ASSERT_THAT(vec_get(paths, 6), Pointee(-1));

    ASSERT_EQ(csfg_graph_find_forward_paths(&g, &paths, n1, n5), 0);
    ASSERT_EQ(vec_count(paths), 4);
    ASSERT_THAT(vec_get(paths, 0), Pointee(e1));
    ASSERT_THAT(vec_get(paths, 1), Pointee(e2));
    ASSERT_THAT(vec_get(paths, 2), Pointee(e6));
    ASSERT_THAT(vec_get(paths, 3), Pointee(-1));
}
