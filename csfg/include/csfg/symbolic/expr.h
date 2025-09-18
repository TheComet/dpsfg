#pragma once

#include "csfg/util/strlist.h"

enum csfg_expr_type
{
    CSFG_EXPR_LITERAL,
    CSFG_EXPR_VARIABLE,
    CSFG_EXPR_INFINITY,
    CSFG_EXPR_OP,
};

struct csfg_expr_node
{
    union
    {
        double         lit;
        struct strspan ident;
        double (*op)();
    } value;

    int parent;
    int child[2];

    enum csfg_expr_type type;
};

struct csfg_expr
{
    struct strlist* idents;

    int count;
    int capacity;
    int root;

    struct csfg_expr_node nodes[1];
};

void csfg_expr_init(struct csfg_expr** expr);
void csfg_expr_deinit(struct csfg_expr* expr);

/*!
 * @brief Parses a string into a syntax tree. The resulting expression can be
 * evaluated using @see csfg_expr_eval();
 * @return If parsing fails, NULL is returned.
 */
int csfg_expr_parse(struct csfg_expr** expr, const char* text);

/*!
 * @brief Parses a string into a syntax tree. The resulting expression can be
 * evaluated using @see csfg_expr_eval();
 * @return If parsing fails, NULL is returned.
 */
double csfg_expr_eval(const struct csfg_expr* expr);

int csfg_expr_lit(struct csfg_expr** expr, double value);
int csfg_expr_add(struct csfg_expr** expr, int left, int right);
int csfg_expr_sub(struct csfg_expr** expr, int left, int right);
int csfg_expr_mul(struct csfg_expr** expr, int left, int right);
int csfg_expr_div(struct csfg_expr** expr, int left, int right);
int csfg_expr_pow(struct csfg_expr** expr, int base, int exp);
