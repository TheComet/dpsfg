#include "csfg/symbolic/expr_op.h"
#include <stdarg.h>
#include <stddef.h>

typedef int (*pass_func)(struct csfg_expr_pool**);

int csfg_expr_op_run_until_complete(struct csfg_expr_pool** pool, ...)
{
    va_list   ap;
    pass_func pass;
    int       pass_modified, modified;

    modified = 0;
again:
    pass_modified = 0;
    va_start(ap, pool);
    while (1)
    {
        pass = va_arg(ap, pass_func);
        if (pass == NULL)
            break;
        switch (pass(pool))
        {
            case -1: return -1;
            case 0: break;
            case 1:
                pass_modified = 1;
                modified = 1;
                break;
        }
    }
    va_end(ap);

    if (pass_modified)
        goto again;

    return 0;
}
