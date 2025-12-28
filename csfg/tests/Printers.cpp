#include "csfg/tests/Printers.hpp"

extern "C" {
#include "csfg/numeric/complex.h"
}

std::ostream& operator<<(std::ostream& os, const struct csfg_complex& c)
{
    os << "(real: " << c.real << ", imag: " << c.imag << ")";
    return os;
}
