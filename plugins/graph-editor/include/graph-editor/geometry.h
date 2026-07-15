#pragma once

/*! Calculates the center position and radius of the circle passing through 3
 * points.
 * \return If successful, returns either -1 or 1 depending on the orientation,
 * and cx,cy,radius will be written. The orientation can be used to correctly
 * calculate the angles for an arc along the circle.
 * If unsuccessful, returns 0. This happens when the circle is very large, or if
 * two points lie on top of each other. You may wan to use \see
 * calc_circle_two_points or \see calc_circle_two_points_overlapping as a
 * backup in this case. cx,cy,radius are unmodified in this case. */
int calc_circle(
    double* cx,
    double* cy,
    double* radius,
    double x1,
    double y1,
    double x2,
    double y2,
    double x3,
    double y3);

/*! Calculates the center position and radius of the circle passing through both
 * points.
 * \return Always returns 1. */
int calc_circle_two_points(
    double* cx,
    double* cy,
    double* radius,
    double x1,
    double y1,
    double x2,
    double y2);

/*! Same as \see calc_circle_two_points, but finds the two overlapping points.
 * This is used when \see calc_circle() fails, and we are not yet sure which two
 * points are overlapping.
 * \return Returns 1 if successful. If no two points are overlapping, then this
 * returns 0 and cx,cy,radius are unmodified. */
int calc_circle_two_points_overlapping(
    double* cx,
    double* cy,
    double* radius,
    double x1,
    double y1,
    double x2,
    double y2,
    double x3,
    double y3);
