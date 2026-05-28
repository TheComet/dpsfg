#pragma once

#include "csfg/symbolic/rules.h"
#include "csfg/util/hmap_str.h"
#include "csfg/util/strview.h"
#include "csfg/util/vec.h"

struct csfg_expr_pool;

struct csfg_ruleset
{
    csfg_rule_run_func extern_run;         /* Set to NULL if not used */
    int                next;               /* Index into rulesets[] */
    int                child;              /* Index into rulesets[] */
    int                expr_from, expr_to; /* Index into pool[] */
    unsigned           ignore : 1;
};
VEC_DECLARE(csfg_ruleset_vec, struct csfg_ruleset, 16)
HMAP_DECLARE_STR(extern, csfg_ruleset_hmap, int, 16)

struct csfg_rulebook
{
    struct csfg_expr_pool*    pool;
    struct csfg_ruleset_vec*  rulesets;
    struct csfg_ruleset_hmap* ruleset_map; /* Index into rulesets[] */
};

void csfg_rulebook_init(struct csfg_rulebook* book);
void csfg_rulebook_deinit(struct csfg_rulebook* book);

/*!
 * \brief Loads a set of rules from a text file.
 * \param book Rulebook to append the rulesets to. It's valid to call this
 * function multiple times. Rulesets are accumulated into the rulebook.
 * \param[in] filename This is only used for error messages and doesn't have to
 * match the actual filename.
 * \param[in] text The contents of the file to parse.
 */
int csfg_rulebook_parse(
    struct csfg_rulebook* book, const char* filename, struct strview text);

int csfg_rulebook_run(
    const struct csfg_rulebook* book,
    const char*                 ruleset_name,
    struct csfg_expr_pool**     pool,
    int*                        expr);
