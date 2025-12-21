#include "csfg/numeric/poly.h"
#include "csfg/symbolic/expr.h"
#include "csfg/symbolic/poly_expr.h"

VEC_DEFINE(csfg_cpoly, struct csfg_complex, 8)
VEC_DEFINE(csfg_rpoly, struct csfg_complex, 8)
VEC_DEFINE(csfg_pfd_poly, struct csfg_pfd, 8)

/* -------------------------------------------------------------------------- */
int csfg_cpoly_from_symbolic(
    struct csfg_cpoly**          poly,
    struct csfg_expr_pool*       pool,
    const struct csfg_var_table* vt,
    const struct csfg_poly_expr* symbolic)
{
    const struct csfg_coeff_expr* coeff;

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

/* -------------------------------------------------------------------------- */
struct csfg_complex
csfg_cpoly_eval(const struct csfg_cpoly* poly, struct csfg_complex s)
{
    int                        i;
    const struct csfg_complex* coeff;

    struct csfg_complex param = csfg_complex(1.0, 0.0);
    struct csfg_complex result = csfg_complex(0.0, 0.0);
    vec_enumerate (poly, i, coeff)
    {
        result = csfg_complex_add(result, csfg_complex_mul(*coeff, param));
        param = csfg_complex_mul(param, s);
    }

    return result;
}
