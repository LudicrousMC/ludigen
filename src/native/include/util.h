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

// translates a value from one range to another
static inline double clamped_map(
    double pos1,
    double x,
    double y,
    double val1,
    double val2
) {
    double inv_lerp = inverse_lerp(pos1, x, y);
    return inv_lerp < 0.0 ? val1 : inv_lerp > 1.0 ? val2 : lerp(inv_lerp, val1, val2);
}

// linearly extends if derivative is non-zero
static inline float linear_ext_if_non_zero(float p_deriv, float p_loc, float x, float value) {
    return p_deriv == 0.0 ? value : value + (p_deriv * (x - p_loc));
}

// interpolates between two spline points
static inline float spline_compute(float x_loc, float x_deriv, float x_val, float y_loc, float y_deriv, float y_val, float coord) {
    float loc_dist = y_loc - x_loc;
    float pos = (coord - x_loc) / loc_dist;
    float val_dist = y_val - x_val;
    float val1 = x_deriv * loc_dist - val_dist;
    float val2 = -y_deriv * loc_dist + val_dist;
    return lerp(pos, x_val, y_val) + (pos * (1.0 - pos)) * lerp(pos, val1, val2);
}

static inline int pos_mod(int a, int b) {
    return (a % b + b) % b;
}

#endif