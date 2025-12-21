#include "csfg/numeric/poly.h"
#include <math.h>

/* -------------------------------------------------------------------------- */
double
csfg_pfd_poly_eval_inverse_laplace(const struct csfg_pfd_poly* pfd, double t)
{
    const struct csfg_pfd* term;

    /*
     *     A
     *   -----   o--o   E(t) * A*e^(p*t)
     *   s - p
     *
     *   Re(A*e^(p*t)) = e^(Re(p)*t) * (Re(A)*cos(Im(p)*t) - Im(A)*sin(Im(p)*t)
     */

    double value = 0.0;
    vec_for_each (pfd, term)
    {
        double cos_ = term->A.real * cos(term->p.imag * t);
        double sin_ = term->A.imag * sin(term->p.imag * t);
        double exp_ = exp(term->p.real * t);
        value += exp_ * (cos_ - sin_);
    }

    return value;
}
