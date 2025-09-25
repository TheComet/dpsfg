#pragma once

#include "csfg/util/strlist.h"

struct csfg_var_table;

enum csfg_expr_type
{
    CSFG_EXPR_GC,
    CSFG_EXPR_LIT,
    CSFG_EXPR_VAR,
    CSFG_EXPR_INF,
    CSFG_EXPR_NEG,
    CSFG_EXPR_OP_ADD,
    CSFG_EXPR_OP_MUL,
    CSFG_EXPR_OP_POW,
};

struct csfg_expr_node
{
    union
    {
        double lit;
        int    var_idx;
    } value;

    int child[2];

    enum csfg_expr_type type : 5;
    unsigned            visited : 1;
};

struct csfg_expr_pool
{
    struct strlist* var_names;

    int count;
    int capacity;

    struct csfg_expr_node nodes[1];
};

void csfg_expr_pool_init(struct csfg_expr_pool** pool);
void csfg_expr_pool_deinit(struct csfg_expr_pool* pool);
void csfg_expr_pool_clear(struct csfg_expr_pool* pool);

/*!
 * @brief Parses a string into a syntax tree. The resulting expression can be
 * evaluated using @see csfg_expr_eval();
 *
 * Grammar:
 *    <expr>   = <term> {("+" | "-") <term>}
 *    <term>   = <unary> {("*" | "/") <unary>}
 *    <unary>  = {("-" | "+")} <factor>
 *    <factor> = <base> {"^" <unary>}
 *    <base>   = <constant> | <variable> | "(" <expr> ")"
 *
 * @return Returns the root node index of the expression tree, or a negative
 * error code if an error occurred.
 */
int csfg_expr_parse(struct csfg_expr_pool** pool, const char* text);

/*!
 * @brief Evaluates the syntax tree and computes a numerical value.
 * @param[in] vt Optional variable table. This is required if your tree has
 * variables in it. If it does not, then you can pass NULL here.
 * @return If anything goes wrong, NaN is returned.
 */
double csfg_expr_eval(
    struct csfg_expr_pool* pool, int root, const struct csfg_var_table* vt);

/*! Main function used to allocate a new node */
int csfg_expr_new(
    struct csfg_expr_pool** pool,
    enum csfg_expr_type     type,
    int                     left,
    int                     right);

/* Leaf nodes */
int csfg_expr_lit(struct csfg_expr_pool** pool, double value);
int csfg_expr_var(struct csfg_expr_pool** pool, struct strview name);
int csfg_expr_inf(struct csfg_expr_pool** pool);

int csfg_expr_set_lit(struct csfg_expr_pool** pool, int n, double value);
int csfg_expr_set_var(struct csfg_expr_pool** pool, int n, struct strview name);
int csfg_expr_set_inf(struct csfg_expr_pool** pool, int n);

/* Unary operators */
int csfg_expr_neg(struct csfg_expr_pool** pool, int child);
int csfg_expr_set_neg(struct csfg_expr_pool** pool, int n, int child);

/* Binary operators */
int csfg_expr_binop(
    struct csfg_expr_pool** pool,
    enum csfg_expr_type     type,
    int                     left,
    int                     right);
int csfg_expr_add(struct csfg_expr_pool** pool, int left, int right);
int csfg_expr_sub(struct csfg_expr_pool** pool, int left, int right);
int csfg_expr_mul(struct csfg_expr_pool** pool, int left, int right);
int csfg_expr_div(struct csfg_expr_pool** pool, int left, int right);
int csfg_expr_pow(struct csfg_expr_pool** pool, int base, int exp);

int csfg_expr_set_binop(
    struct csfg_expr_pool** pool,
    int                     n,
    enum csfg_expr_type     type,
    int                     left,
    int                     right);
int csfg_expr_set_add(struct csfg_expr_pool** pool, int n, int left, int right);
int csfg_expr_set_mul(struct csfg_expr_pool** pool, int n, int left, int right);
int csfg_expr_set_pow(struct csfg_expr_pool** pool, int n, int base, int exp);

/* Recursively duplicate a subtree */
int csfg_expr_dup_from(
    struct csfg_expr_pool** dst, const struct csfg_expr_pool* src, int n);
int csfg_expr_dup(struct csfg_expr_pool** pool, int n);
/* Copy a single node. All fields. */
int csfg_expr_copy(struct csfg_expr_pool** pool, int n);

/* Mark for deletion (does not modify any other nodes, i.e. iterators remain
 * valid). csfg_expr_gc() removes all marked nodes and shrinks the array.
 * Indices will be modified, including potentially the root node. The new root
 * node is returned. These functions cannot fail. */
void csfg_expr_mark_deleted(struct csfg_expr_pool* pool, int n);
void csfg_expr_mark_deleted_recursive(struct csfg_expr_pool* pool, int n);
int  csfg_expr_gc(struct csfg_expr_pool* pool, int root);

/* Overwrite the parent node with the child, and mark the sibling tree as
 * deleted. Make sure to call csfg_expr_gc() to clean up. */
void csfg_expr_collapse_into_parent(
    struct csfg_expr_pool* pool, int child, int parent);
void csfg_expr_collapse_sibling_into_parent(struct csfg_expr_pool* pool, int n);

/* Returns the parent node if it exists, or -1. Ignores nodes marked for GC */
int csfg_expr_find_parent(const struct csfg_expr_pool* pool, int n);
int csfg_expr_find_sibling(const struct csfg_expr_pool* pool, int n);

/* Compares (recursively) if two subtrees match. Node indices can be different,
 * but the structure and the values contained within the nodes must match.
 * Returns true or false. */
int csfg_expr_equal(
    const struct csfg_expr_pool* p1,
    int                          root1,
    const struct csfg_expr_pool* p2,
    int                          root2);
