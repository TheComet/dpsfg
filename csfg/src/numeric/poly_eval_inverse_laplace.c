#include "csfg/numeric/poly.h"
#include <math.h>

/* -------------------------------------------------------------------------- */
static double factorial(int n)
{
    double result = 1.0;
    for (; n >= 1; --n)
        result *= (double)n;
    return result;
}

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
     *      A                    t^(n-1) * A*e^(p*t^(n-1))
     *   -------   o--o   E(t) * -------------------------
     *   (s-p)^n                        (n-1)!
     *
     * Re(A*e^(p*t)) = e^(Re(p)*t) * (Re(A)*cos(Im(p)*t) - Im(A)*sin(Im(p)*t)
     */

    double value = 0.0;
    vec_for_each (pfd, term)
    {
        if (term->n == 1)
        {
            double cos_ = term->A.real * cos(term->p.imag * t);
            double sin_ = term->A.imag * sin(term->p.imag * t);
            double exp_ = exp(term->p.real * t);
            value += exp_ * (cos_ - sin_);
        }
        else
        {
            double tn = (term->n - 1);
            double tpow = pow(t, tn);
            double fact = factorial(term->n - 1);
            double cos_ = term->A.real * cos(term->p.imag * tpow);
            double sin_ = term->A.imag * sin(term->p.imag * tpow);
            double exp_ = exp(term->p.real * tpow);
            value += tpow / fact * exp_ * (cos_ - sin_);
        }
    }

    return value;
}
