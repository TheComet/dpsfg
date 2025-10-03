#pragma once

struct csfg_expr_pool;
struct csfg_var_hmap;

struct csfg_var_table
{
    struct csfg_expr_pool* pool;
    struct csfg_var_hmap*  map;
};

void csfg_var_table_init(struct csfg_var_table* vt);
void csfg_var_table_deinit(struct csfg_var_table* vt);

/*!
 * @brief Collects all variables in the expression and inserts them into the
 * variable table.
 * @note The variables are mapped to default values of either 1.0 if the
 * parent operation is mul, div or pow, or 0.0 if anything else.
 */
int csfg_var_table_populate(
    struct csfg_var_table* vt, const struct csfg_expr_pool* pool, int root);

/*!
 * @brief Adds a new entry to the table that maps "name" to the constant of
 * "value". If the entry already exists, then the existing entry is
 * modified.
 * @note The value is stored as a constant expression (csfg_expr).
 */
int csfg_var_table_set_lit(
    struct csfg_var_table* vt, const char* name, double value);

/*!
 * @brief Adds a new entry to the table that maps "name" to an expression.
 * If the entry already exists, then the existing entry is replaced.
 * @note The variable table takes ownership of the expression, and will
 * deallocate it in @see csfg_var_table_deinit().
 */
int csfg_var_table_set_expr(
    struct csfg_var_table* vt,
    const char*            name,
    struct csfg_expr_pool* pool,
    int                    root);
int csfg_var_table_set_parse_expr(
    struct csfg_var_table* vt, const char* name, const char* expr);

/*!
 * @brief Retrieves the expression the specified variable would be mapped to,
 * or NULL if no entry exists.
 */
struct csfg_expr_pool* csfg_var_table_get(
    const struct csfg_var_table* vt, const char* name, int* root);

/*!
 * @brief Recursively evaluates the expression the specified variable maps
 * to until a constant value is computed.
 */
double csfg_var_table_eval(const struct csfg_var_table* vt, const char* name);
