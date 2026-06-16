#ifndef OP_STACK_H
#define OP_STACK_H

// Use fixed width types and forward-declare structs if called from c code
#ifndef SHADER
#include <stdint.h>
typedef struct StackFrame StackFrame;
typedef struct EvalState EvalState;
#endif

struct StackFrame {
    uint8_t op_type;
    uint8_t arg_types[3];
    uint16_t arg_indexes[3];
    uint16_t const_index;
    uint8_t noise_index;
    uint8_t result_index;
};

struct EvalState {
    int x;
    int y;
    int z;
    int stack_offset;
    double result_register[80];
};

#endif