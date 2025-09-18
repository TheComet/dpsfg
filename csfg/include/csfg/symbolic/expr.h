#pragma once

#include "csfg/util/strlist.h"

struct csfg_var_table;

enum csfg_expr_type
{
    CSFG_EXPR_LIT,
    CSFG_EXPR_VAR,
    CSFG_EXPR_INF,
    CSFG_EXPR_OP_ADD,
    CSFG_EXPR_OP_SUB,
    CSFG_EXPR_OP_MUL,
    CSFG_EXPR_OP_DIV,
    CSFG_EXPR_OP_POW,
};

struct csfg_expr_node
{
    union
    {
        double lit;
        int    var_idx;
    } value;

    int parent;
    int child[2];

    enum csfg_expr_type type : 5;
    unsigned            visited : 1;
};

struct csfg_expr
{
    struct strlist* var_names;

    int count;
    int capacity;
    int root;

    struct csfg_expr_node nodes[1];
};

void csfg_expr_init(struct csfg_expr** expr);
void csfg_expr_deinit(struct csfg_expr* expr);
void csfg_expr_clear(struct csfg_expr* expr);

int csfg_expr_lit(struct csfg_expr** expr, double value);
int csfg_expr_var(struct csfg_expr** expr, struct strview name);

int csfg_expr_binop(
    struct csfg_expr** expr, enum csfg_expr_type type, int left, int right);
int csfg_expr_add(struct csfg_expr** expr, int left, int right);
int csfg_expr_sub(struct csfg_expr** expr, int left, int right);
int csfg_expr_mul(struct csfg_expr** expr, int left, int right);
int csfg_expr_div(struct csfg_expr** expr, int left, int right);
int csfg_expr_pow(struct csfg_expr** expr, int base, int exp);

/*!
 * @brief Parses a string into a syntax tree. The resulting expression can be
 * evaluated using @see csfg_expr_eval();
 * @return If parsing fails, NULL is returned.
 */
int csfg_expr_parse(struct csfg_expr** expr, const char* text);

/*!
 * @brief Evaluates the syntax tree and computes a numerical value.
 * @param[in] vt Optional variable table. This is required if your tree has
 * variables in it. If it does not, then you can pass NULL here.
 * @return If anything goes wrong, NaN is returned.
 */
double csfg_expr_eval(struct csfg_expr* expr, const struct csfg_var_table* vt);
