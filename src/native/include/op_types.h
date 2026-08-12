#ifndef OP_TYPES_H
#define OP_TYPES_H

// Operation Const not used during stack iteration
// but used for node types during stack gen
#define OP_CONST                0
#define OP_ADD                  1
#define OP_MUL                  2
#define OP_MIN                  3
#define OP_MAX                  4
#define OP_ABS                  5
#define OP_SQUARE               6
#define OP_CUBE                 7
#define OP_HALF_NEGATIVE        8
#define OP_QUARTER_NEGATIVE     9
#define OP_INVERT               10
#define OP_SQUEEZE              11
#define OP_CLAMP                12
#define OP_Y_CLAMPED_GRAD       13
#define OP_FIND_TOP_SURFACE     14
#define OP_RANGE_CHOICE         15
#define OP_INTERVAL_SELECT      16
#define OP_NOISE                17
#define OP_SHIFTED_NOISE        18
#define OP_SPLINE               19
#define OP_BLEND_DENSITY        20
#define OP_BLEND_OFFSET         21
#define OP_BLEND_ALPHA          22
#define OP_INTERPOLATED         23
#define OP_CACHE_ONCE           24
#define OP_FLAT_CACHE           25
#define OP_CACHE_2D             26
#define OP_CACHE_ALL_IN_CELL    27
#define OP_SHIFT_A              28
#define OP_SHIFT_B              29
#define OP_OLD_BLENDED_NOISE    30
#define OP_END_ISLANDS          31
// New operations go here and increment OP_HALT
#define OP_HALT                 32

#define OP_SUBTYPE_SPLINE       0
#define OP_SUBTYPE_SPLINE_POINT 1

#endif