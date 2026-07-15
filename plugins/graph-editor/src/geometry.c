#include "graph-editor/geometry.h"
#include <math.h>

/* -------------------------------------------------------------------------- */
int calc_circle(
    double* cx,
    double* cy,
    double* radius,
    double x1,
    double y1,
    double x2,
    double y2,
    double x3,
    double y3)
{
    double dx   = x1 - x3;
    double dy   = y1 - y3;
    double dist = sqrt(dx * dx + dy * dy);

    /* Circumcenter */
    double A   = x1 - x2;
    double B   = y1 - y2;
    double C   = x1 - x3;
    double D   = y1 - y3;
    double E   = ((x1 * x1 - x2 * x2) + (y1 * y1 - y2 * y2)) / 2.0;
    double F   = ((x1 * x1 - x3 * x3) + (y1 * y1 - y3 * y3)) / 2.0;
    double den = A * D - B * C;

    if (fabs(den) < 0.0001)
        return 0;

    *cx     = (D * E - B * F) / den;
    *cy     = (-C * E + A * F) / den;
    *radius = hypot(*cx - x1, *cy - y1);
    if (*radius > 100 * dist)
        return 0;

    double vx1 = (x2 - x1), vy1 = (y2 - y1);
    double vx2 = (x3 - x1), vy2 = (y3 - y1);
    double cross = (vx1 * vy2 - vy1 * vx2);
    return cross > 0 ? 1 : -1;
}

/* -------------------------------------------------------------------------- */
int calc_circle_two_points(
    double* cx,
    double* cy,
    double* radius,
    double x1,
    double y1,
    double x2,
    double y2)
{
    double dx = x1 - x2;
    double dy = y1 - y2;
    *cx       = (x1 + x2) / 2;
    *cy       = (y1 + y2) / 2;
    *radius   = sqrt(dx * dx + dy * dy) / 2;
    return 1;
}

/* -------------------------------------------------------------------------- */
int calc_circle_two_points_overlapping(
    double* cx,
    double* cy,
    double* radius,
    double x1,
    double y1,
    double x2,
    double y2,
    double x3,
    double y3)
{
    if (x1 == x2 && y1 == y2 && (x1 != x3 || y1 != y3))
        return calc_circle_two_points(cx, cy, radius, x1, y1, x3, y3);
    if (x1 == x3 && y1 == y3 && (x1 != x2 || y1 != y2))
        return calc_circle_two_points(cx, cy, radius, x1, y1, x2, y2);
    if (x2 == x3 && y2 == y3 && (x1 != x2 || y1 != y2))
        return calc_circle_two_points(cx, cy, radius, x1, y1, x2, y2);
    return 0;
}
