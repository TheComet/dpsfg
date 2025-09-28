#pragma once

struct str;

struct csfg_node
{
    struct str* name;

    int outgoing;
    int incoming;
};

struct csfg_edge
{
    struct csfg_expr_pool* expr;

    int source;
    int target;
};

struct csfg_graph
{
    int ncount, ncapacity;
    int ecount, ecapacity;

    struct csfg_node nodes[1];
};
