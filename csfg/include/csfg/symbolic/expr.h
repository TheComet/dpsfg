#pragma once

#include "csfg/util/strlist.h"
#include "csfg/util/vec.h"

VEC_DECLARE(csfg_expr_vec, int, 8)

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

int csfg_expr_lit(struct csfg_expr_pool** pool, double value);
int csfg_expr_var(struct csfg_expr_pool** pool, struct strview name);

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

/*!
 * @brief Parses a string into a syntax tree. The resulting expression can be
 * evaluated using @see csfg_expr_eval();
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
    struct csfg_expr_pool* pool, int expr, const struct csfg_var_table* vt);

/*!
 * @brief Attempts to re-arrange the expression into the standard "Transfer
 * function form" and returns the expr index of the division operator (the new
 * root of the expression tree):
 *
 *          b0 + b1*s^1 + b2*s^2 + ...
 *   T(s) = --------------------------
 *          a0 + a1*s^1 + a2*s^2 + ...
 *
 * There are all sorts of reasons why this might fail, but if it succeeds, it
 * means each polynomial coefficient can be extracted using @see
 * csfg_expr_standard_tf_to_coeff(), which allows you to change variables and
 * re-evaluate the new coefficient values without needing to do this
 * transformation each time.
 *
 * If this function doesn't succeed, which will be the case if any term
 * that contains the specified variable (usually "s") is combined with an
 * operation that is *not* mul/div/add/sub with a variable operand (such as
 * the expression s^a) -- or in other words, if the operation containing
 * the specified variable is not reducible to polynomial form -- the
 * expression will be in a modified state different from the initial
 * condition, but mathematically equivalent. In this case, no expressions
 * exist for the polynomial coefficients and the transfer function will
 * have to perform this manipulation every time a variable changes.
 *
 * It is worth noting that DPSFGs will always produce expressions reducible
 * to polynomial expressions. User input may not.
 * @param[in] variable This is the dependent variable, usually "s".
 */
int csfg_expr_to_standard_tf(
    struct csfg_expr_pool** pool, int expr, struct strview variable);

/*!
 * @brief Given an expression in standard "Transfer function form", this
 * function returns two arrays containing the expressions for each coefficient
 * of the numerator and denominator polynomials. @see
 * csfg_expr_to_standard_tf().
 *
 * Assumed Input:
 *
 *          b0 + b1*s^1 + b2*s^2 + ...
 *   T(s) = --------------------------
 *          a0 + a1*s^1 + a2*s^2 + ...
 *
 * Output:
 *
 *   num = [b0, b1, b2, ...]
 *   den = [a0, a1, a2, ...]
 *
 * The coefficients are required to build a transfer_function object, which is
 * required for calculating impulse/step responses, frequency responses,
 * pole-zero diagrams, etc.
 * @param[in] variable This is the dependent variable, usually "s".
 */
int csfg_expr_standard_tf_to_coeff(
    struct csfg_expr_pool** pool,
    int                     expr,
    struct strview          variable,
    struct csfg_expr_vec**  num,
    struct csfg_expr_vec**  den);
