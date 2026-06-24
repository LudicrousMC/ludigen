#ifndef STACK_BUILDER_H
#define STACK_BUILDER_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "op_types.h"
#include "op_stack.h"

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
    double          value;
    struct Node**   allChildren;
} Node;

int max(int *arr, int n); // returns the max from an array of n numbers
void register_op(int op_type, int priority, int associative); // registers a new operation
OpInfo* lookup_op(int op_type); // looks up an operation
int preorder_traversal_depth_count(Node* root); // preorder traversal of tree and labelling of depths
EvalStackAllocReqs preorder_traversal_alloc_reqs(Node* root); // preorder traversal to determine allocation sizes of EvalStack arrays
int generate_stack_recursive(EvalStack* evalStack, Node* node, int regPos); // recursive function to generate evaluation stack
EvalStack* generate_stack_wrapper(Node* node); // wrapper function to generate evaluation stack
int one_arg_op_gen(EvalStack* evalStack, size_t currentFrameIndex, Node* node, int regPos); // generates stack for a one arg operation
int two_args_op_gen(EvalStack* evalStack, size_t currentFrameIndex, Node* node, int regPos); // generates stack for a two arg operation
int three_args_op_gen(EvalStack* evalStack, size_t currentFrameIndex, Node* node, int regPos); // generates stack for a three arg operation
Node* create_leaf(int op_type, double theData); // creates a leaf node (always constant) with data
Node* create_op(int op_type, Node** theChildren); // creates an operation node with children
int rotation_is_safe(Node* rest); // CLAUDE FUNCTION - determines whether or not a child is safe to rotate
void rotate_right(Node* tail); // CLAUDE FUNCTION - rotates a child node
int larger_arg_right(Node* root); // swap children with the largest args to the right
Node* tree_to_vine(Node* root); // CLAUDE FUNCTION - "reassociation" optimization, which smashes a tree into a linked list
double eval_stack(EvalStack* finalStack); // evaluates the stack given the inputs

#endif