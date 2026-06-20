#ifndef STACK_BUILDER_H
#define STACK_BUILDER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include ".\include\op_types.h"
#include ".\include\op_stack.h"
#include "stack_builder.h"

typedef struct OpInfo {
    int             op_type;
    int             priority;      // higher number = tighter binding
    int             associative;   // 1 = safe to rotate, 0 = do not rotate
} OpInfo;

typedef struct EvalStack {
    StackFrame** op_stack;
    int op_stack_len;
    int max_reg;
    double* const_args;
    int const_args_len;
} EvalStack;

typedef enum { NODE_LEAF, NODE_OP } NodeType;

typedef struct Node {
    NodeType        type;
    int             operationType;
    int (*generate_stack_recursive_from_op)(EvalStack* evalStack, StackFrame* currentNode, struct Node* node, int regPos);
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
int generate_stack_recursive(EvalStack* evalStack, Node* node, int regPos); // recursive function to generate evaluation stack
EvalStack* generate_stack_wrapper(Node* node); // wrapper function to generate evaluation stack
int two_args_op_gen(EvalStack* evalStack, StackFrame* currentNode, Node* node, int regPos); // generates stack for a two arg operation
int three_args_op_gen(EvalStack* evalStack, StackFrame* currentNode, Node* node, int regPos); // generates stack for a three arg operation
Node* create_leaf(int op_type, double theData); // creates a leaf node (always constant) with data
Node* create_op(int op_type, Node** theChildren); // creates an operation node with children
int rotation_is_safe(Node* rest); // CLAUDE FUNCTION - determines whether or not a child is safe to rotate
void rotate_right(Node* tail); // CLAUDE FUNCTION - rotates a child node
Node* tree_to_vine(Node* root); // CLAUDE FUNCTION - "reassociton" optimization, which smashes a tree into a linked list
Node* larger_opt_test_case(); // generates a test case for larger branch last optimization: 1 reg used
Node* larger_opt_test_case_shifted_noise1(); // generates a test case for larger branch last optimization with shifted noise: 2 regs used
Node* larger_opt_test_case_shifted_noise2(); // generates a test case for larger branch last optimization with shifted noise: 3 regs used
Node* non_associativity_test_case(); // generates a simple test case for non associativity: 2 regs used
double eval_stack(EvalStack* finalStack); // evaluates the stack given the inptus

#define MAX_OPS 16
OpInfo op_table[MAX_OPS];
int    op_count = 0;

#endif