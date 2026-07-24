#include "util.h"

double clamped_map(
    double pos1,
    double x,
    double y,
    double val1,
    double val2
) {
    double inv_lerp = inverse_lerp(pos1, x, y);
    return inv_lerp < 0.0 ? val1 : inv_lerp > 1.0 ? val2 : lerp(inv_lerp, val1, val2);
}

double linear_ext_if_non_zero(double p_deriv, double p_loc, double x, double value) {
    return p_deriv == 0.0 ? value : value + (p_deriv * (x - p_loc));
}

double spline_compute(double x_loc, double x_deriv, double x_val, double y_loc, double y_deriv, double y_val, double coord) {
    double loc_dist = y_loc - x_loc;
    double pos = (coord - x_loc) / loc_dist;
    double val_dist = y_val - x_val;
    double val1 = x_deriv * loc_dist - val_dist;
    double val2 = -y_deriv * loc_dist + val_dist;
    return lerp(pos, x_val, y_val) + (pos * (1.0 - pos)) * lerp(pos, val1, val2);
}