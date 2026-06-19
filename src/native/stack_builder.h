#ifndef STACK_BUILDER_H
#define STACK_BUILDER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "op_stack.h"

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
    double          value;
    struct Node*    left;
    struct Node*    right;
} Node;

typedef struct GenerateStackReturn {
    int returnedNeededRegs;
    int returnedIndex;
    StackFrame* returnedStackFrames;
    EvalStack* returnedEvalStack;
} GenerateStackReturn;

int max(int *arr, int n);
void register_op(int op_type, int priority, int associative);
OpInfo* lookup_op(int op_type);
int preorder_traversal_depth_count(Node* root);
int generate_stack_recursive(EvalStack* evalStack, Node* node, int regPos);
EvalStack* generate_stack_wrapper(Node* node);
int two_args_op_gen(EvalStack* evalStack, StackFrame* currentNode, Node* node, int regPos);
Node* create_leaf(int op_type, double theData);
Node* create_op(int op_type, Node* left, Node* right);
int rotation_is_safe(Node* rest);
void rotate_right(Node* tail);
Node* tree_to_vine(Node* root);
Node* larger_opt_test_case();
Node* non_associativity_test_case();

#define MAX_OPS 16
OpInfo op_table[MAX_OPS];
int    op_count = 0;

#endif