#include <stdbool.h>
#include "op_types.h"
#include "op_stack.h"
#include "stack_builder.h"
#include "util.h"

double lerp(double pos, double x, double y) {
    return x + (pos * (y - x));
}

double inverse_lerp(double pos, double x, double y) {
    return (pos - x) / (y - x);
}

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