#include "csfg/config.h"
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/rulebook.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#define DEBUG_PRINTF 1

/* -------------------------------------------------------------------------- */
void csfg_rulebook_init(struct csfg_rulebook* book)
{
    csfg_ruleset_hmap_init(&book->ruleset_map);
    csfg_expr_pool_init(&book->pool);
    csfg_ruleset_vec_init(&book->rulesets);
}

/* -------------------------------------------------------------------------- */
void csfg_rulebook_deinit(struct csfg_rulebook* book)
{
    csfg_ruleset_vec_deinit(book->rulesets);
    csfg_expr_pool_deinit(book->pool);
    csfg_ruleset_hmap_deinit(book->ruleset_map);
}
