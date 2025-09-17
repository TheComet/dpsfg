#pragma once

enum csfg_expr_type
{
    CSFG_EXPR_LITERAL,
    CSFG_EXPR_VARIABLE,
    CSFG_EXPR_INFINITY,
    CSFG_EXPR_OP,
};

enum
{
    CSFG_MAX_CHILDREN = 2
};

union csfg_expr_node
{
    struct base
    {
        int parent;
        int child[CSFG_MAX_CHILDREN];

        enum csfg_expr_type type;
    } base;

    struct
    {
        struct base base;
        float       value;
    } lit;

    struct
    {
        struct base base;
        struct str* name;
    } var;

    struct
    {
        struct base base;
        float (*func)();
    } op;
};

struct csfg_expr
{
    int count;
    int capacity;
    int root;

    union csfg_expr_node nodes[1];
};

/*!
 * @brief Parses a string into a syntax tree. The resulting expression can be
 * evaluated using @see csfg_expr_eval();
 * @return If parsing fails, NULL is returned.
 */
struct csfg_expr* csfg_expr_parse(const char* text);
