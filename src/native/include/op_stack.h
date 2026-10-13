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
    uint8_t result_index;
    // For arg_indexes, positive values represent an index in local result_register.
    // Negative values represent absolute of index - 1 in global const_args array
    int16_t arg_indexes[3];
    // For const_index, a positive value represents a starting index in global const_args array for an ops global constants.
    // Negative value represents an index in global op_stack array of a sub operations main StackFrame.
    // This allows sub ops to find constants through the main StackFrame, it makes identifying sub operations easier,
    // and allows sub ops to link back to their main op
    int16_t const_index;
};

#endif