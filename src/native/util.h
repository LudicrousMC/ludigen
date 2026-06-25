#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include "op_types.h"
#include "op_stack.h"
#include "stack_builder.h"

double lerp(double pos, double x, double y); // linear interpolation
double inverse_lerp(double pos, double x, double y); // inverse linear interpolation
double clamped_map(double pos1, double x, double y, double val1, double val2); // translates a value from one range to another

#endif