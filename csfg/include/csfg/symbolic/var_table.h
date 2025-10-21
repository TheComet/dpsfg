#pragma once

#include "csfg/util/hmap_str.h"
#include "csfg/util/strview.h"

struct csfg_expr_pool;

struct csfg_var_table_entry
{
    struct csfg_expr_pool* pool;
    int                    expr;
    unsigned               visited : 1;
};

HMAP_DECLARE_STR(extern, csfg_var_hmap, struct csfg_var_table_entry, 16)

struct csfg_var_table
{
    struct csfg_var_hmap* map;
};

void csfg_var_table_init(struct csfg_var_table* vt);
void csfg_var_table_deinit(struct csfg_var_table* vt);
void csfg_var_table_clear(struct csfg_var_table* vt);

/*!
 * @brief Collects all variables in the expression and inserts them into the
 * variable table. If the variable does not already exist, then its value
 * defaults to 1.0. If the variable already exists, then its value is not
 * modified. Variables found in the expression are marked as "visited". If you
 * want to remove unused variables from the table, you can first call @see
 * csfg_var_table_reset_visited(), and after populating the table call @see
 * csfg_var_table_erase_unvisited() to clear any unvisited variables.
 */
int csfg_var_table_populate(
    struct csfg_var_table* vt, const struct csfg_expr_pool* pool, int expr);
void csfg_var_table_reset_visited(struct csfg_var_table* vt);
void csfg_var_table_erase_unvisited(struct csfg_var_table* vt);

/*!
 * @brief Adds a new entry to the table that maps "name" to the constant of
 * "value". If the entry already exists, then the existing entry is
 * modified.
 * @note The value is stored as a constant expression (csfg_expr).
 */
int csfg_var_table_set_lit(
    struct csfg_var_table* vt, struct strview name, double value);

/*!
 * @brief Adds a new entry to the table that maps "name" to an expression.
 * If the entry already exists, then the existing entry is replaced.
 * @note The variable table takes ownership of the expression, and will
 * deallocate it in @see csfg_var_table_deinit().
 */
int csfg_var_table_set_expr(
    struct csfg_var_table* vt,
    struct strview         name,
    struct csfg_expr_pool* pool,
    int                    expr);
int csfg_var_table_set_parse_expr(
    struct csfg_var_table* vt, struct strview name, struct strview expr);

/*!
 * @brief Retrieves the expression the specified variable would be mapped to,
 * or NULL if no entry exists.
 */
struct csfg_expr_pool* csfg_var_table_get(
    const struct csfg_var_table* vt, struct strview name, int* expr);

/*!
 * @brief Recursively evaluates the expression the specified variable maps
 * to until a constant value is computed.
 */
double
csfg_var_table_eval(const struct csfg_var_table* vt, struct strview name);
