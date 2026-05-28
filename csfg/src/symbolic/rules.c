#include "csfg/config.h"
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/rules.h"
#include <assert.h>

/* -------------------------------------------------------------------------- */
int csfg_rule_run(struct csfg_expr_pool** pool, csfg_rule_run_func pass)
{
    int modified = 0;
again:
    switch (pass(pool))
    {
        case -1: return -1;
        case 0: break;
        case 1:
            modified = 1;
            CSFG_DEBUG_ASSERT(
                csfg_expr_integrity_check_allow_islands(*pool) == 0);
            goto again;
    }
    return modified;
}

/* -------------------------------------------------------------------------- */
int csfg_rules_runv(struct csfg_expr_pool** pool, va_list ap)
{
    csfg_rule_run_func pass;
    int                pass_modified, modified;
    va_list            copy;

    modified = 0;
again:
    pass_modified = 0;
    va_copy(copy, ap);
    while (1)
    {
        pass = va_arg(copy, csfg_rule_run_func);
        if (pass == NULL)
            break;
        switch (pass(pool))
        {
            case -1: return -1;
            case 0: break;
            case 1:
                pass_modified = 1;
                modified = 1;
                CSFG_DEBUG_ASSERT(
                    csfg_expr_integrity_check_allow_islands(*pool) == 0);
                break;
        }
    }
    va_end(copy);

    if (pass_modified)
        goto again;

    return modified;
}

/* -------------------------------------------------------------------------- */
int csfg_rules_run(struct csfg_expr_pool** pool, ...)
{
    int     result;
    va_list ap;
    va_start(ap, pool);
    result = csfg_rules_runv(pool, ap);
    va_end(ap);
    return result;
}
