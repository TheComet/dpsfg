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
    struct csfg_expr* expr;

    int source;
    int target;

    int outgoing_next;
    int incoming_next;
};

struct csfg_graph
{
    int ncount, ncapacity;
    int ecount, ecapacity;

    struct csfg_node nodes[1];
};
