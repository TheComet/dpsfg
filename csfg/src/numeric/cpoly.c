#include "csfg/numeric/cpoly.h"
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/poly.h"

/* -------------------------------------------------------------------------- */
int csfg_cpoly_from_symbolic(
    struct csfg_cpoly**          poly,
    struct csfg_expr_pool*       pool,
    const struct csfg_var_table* vt,
    const struct csfg_poly*      symbolic)
{
    const struct csfg_coeff* coeff;

    csfg_cpoly_clear(*poly);
    if (csfg_cpoly_realloc(poly, vec_count(symbolic)) != 0)
        return -1;

    vec_for_each (symbolic, coeff)
    {
        double value = coeff->factor;
        if (coeff->expr > -1)
            value *= csfg_expr_eval(pool, coeff->expr, vt);
        csfg_cpoly_push(poly, csfg_complex(value, 0.0));
    }

    return 0;
}
