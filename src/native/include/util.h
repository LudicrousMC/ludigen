#ifndef UTIL_H
#define UTIL_H

// linear interpolation
static inline double lerp(double pos, double x0, double x1) {
    return x0 + (pos * (x1 - x0));
}
// bilinear interpolation
static inline double lerp2(
    double pos1,
    double pos2,
    double x0,
    double x1,
    double y0,
    double y1
) {
    return lerp(pos2, lerp(pos1, x0, x1), lerp(pos1, y0, y1));
}
// trilinear interpolation
static inline double lerp3(
    double pos1,
    double pos2,
    double pos3,
    double x00,
    double x10,
    double y00,
    double y10,
    double x01,
    double x11,
    double y01,
    double y11
) {
    return lerp(pos3, lerp2(pos1, pos2, x00, x10, y00, y10), lerp2(pos1, pos2, x01, x11, y01, y11));
}
// inverse linear interpolation
static inline double inverse_lerp(double pos, double x, double y) {
    return (pos - x) / (y - x);
}
static inline double smoothstep(double val) {
    return val * val * val * (val * (val * 6.0 - 15.0) + 10.0);
}
double clamped_map(double pos1, double x, double y, double val1, double val2); // translates a value from one range to another
double linear_ext_if_non_zero(double p_deriv, double p_loc, double x, double value); // linearly extends if derivative is non-zero
double spline_compute(double x_loc, double x_deriv, double x_val, double y_loc, double y_deriv, double y_val, double coord); // interpolates between two spline points

#endif