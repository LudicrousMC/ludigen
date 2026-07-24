#ifndef STACK_BUILDER_H
#define STACK_BUILDER_H

#include <stddef.h>
#include "op_stack.h"
#include "noise_gen.h"

typedef struct OpInfo {
    int             op_type;
    int             priority;      // higher number = tighter binding
    int             associative;   // 1 = safe to rotate, 0 = do not rotate
} OpInfo;

typedef struct EvalStack {
    StackFrame* op_stack;
    size_t op_stack_len;
    size_t max_reg;
    double* const_args;
    size_t const_args_len;
    NormalNoise* noise_states;
    ImprovedNoise* noise_levels;
} EvalStack;

typedef struct EvalStackAllocReqs {
    size_t op_stack_len;
    size_t const_args_len;
} EvalStackAllocReqs;

typedef enum { NODE_LEAF, NODE_OP } NodeType;

typedef struct Node {
    NodeType        type;
    int             operationType;
    int (*generate_stack_recursive_from_op)(EvalStack* evalStack, size_t currentFrameIndex, struct Node* node, int regPos);
    int             label;
    int             depth;
    int             numChildren;
    double*         values;
    struct Node**   allChildren;
} Node;

int max(int *arr, int n); // returns the max from an array of n numbers
void register_op(int op_type, int priority, int associative); // registers a new operation
OpInfo* lookup_op(int op_type); // looks up an operation
EvalStack* generate_stack_wrapper(Node* node); // wrapper function to generate evaluation stack
Node* create_leaf(int op_type, double* theData); // creates a leaf node with data
Node* create_op(int op_type, Node** theChildren, double* theData); // creates an operation node with children
int rotation_is_safe(Node* rest); // CLAUDE FUNCTION - determines whether or not a child is safe to rotate
void rotate_right(Node* tail); // CLAUDE FUNCTION - rotates a child node
int larger_arg_right(Node* root); // swap children with the largest args to the right
Node* tree_to_vine(Node* root); // CLAUDE FUNCTION - "reassociation" optimization, which smashes a tree into a linked list
double get_arg(EvalStack* s, double* registers, int offset, int arg_num); // gets argument from const args or register (depending on op children)
double get_const(EvalStack* s, int offset, int arg_num); // gets an op's constant args
double evaluate_stack(EvalStack* finalStack, int x, int y, int z); // evaluates the stack given the inputs and coordinates

#endif