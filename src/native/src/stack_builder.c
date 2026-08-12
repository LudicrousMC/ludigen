#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include "op_types.h"
#include "op_stack.h"
#include "stack_builder.h"
#include "noise_gen.h"
#include "util.h"

#define MAX_OPS 30
OpInfo op_table[MAX_OPS];
int    op_count = 0;

int max(int *arr, int n) {
    int m = arr[0];
    for (int i = 1; i < n; i++)
        if (arr[i] > m) m = arr[i];
    return m;
}

// Register an operator with a priority and associativity flag.
void register_op(int op_type, int priority, int associative) {
    for (int i = 0; i < op_count; i++) {
        if (op_table[i].op_type == op_type) {          // update existing entry
            op_table[i].priority    = priority;
            op_table[i].associative = associative;
            return;
        }
    }
    if (op_count < MAX_OPS) {
        op_table[op_count++] = (OpInfo){ op_type, priority, associative };
    }
}

// Returns NULL if the operator is not registered.
OpInfo* lookup_op(int op_type) {
    for (int i = 0; i < op_count; i++) {
        if (op_table[i].op_type == op_type) return &op_table[i];
    }
    return NULL;
}

static int preorder_traversal_depth_count(Node* root) {
    if (root->numChildren == 0) return 0;
    int finalDepths[root->numChildren];
    for (int i = 0; i < root->numChildren; i++) {
        if (root->allChildren[i]) finalDepths[i] = preorder_traversal_depth_count(root->allChildren[i]);
    }

    root->depth = max(finalDepths, root->numChildren);
    return root->depth + 1;
}

// Finds allocation requirements of op_stack and const_args for EvalStack
static EvalStackAllocReqs preorder_traversal_alloc_reqs(Node* root) {
    // Return requirements for leaf nodes that don't have children
    switch (root->operationType)
    {
    case OP_CONST:
        return (EvalStackAllocReqs){0,1};
    case OP_Y_CLAMPED_GRAD:
        return (EvalStackAllocReqs){1,4};
    case OP_NOISE:
        return (EvalStackAllocReqs){1,3};
    case OP_SHIFT_A:
    case OP_SHIFT_B:
        return (EvalStackAllocReqs){1,1};
    case OP_OLD_BLENDED_NOISE:
        return (EvalStackAllocReqs){1,5};
    default:
        break;
    }

    // If this node is an actual operation, reserve 1 length in op_stack
    EvalStackAllocReqs reqs = {1,0};
    // Account for ops that have extra constants
    switch (root->operationType)
    {
    case OP_CLAMP:
        reqs.const_args_len += 2;
        break;
    case OP_SPLINE:
        if (root->values[0] == OP_SUBTYPE_SPLINE_POINT) {
            if (root->allChildren[0]->operationType == OP_CONST)
                return (EvalStackAllocReqs){0,4}; // Omit spline point op if it holds a constant value
            reqs.const_args_len += 3; // derivative, location, and stack index constants
        } else if (root->values[0] == OP_SUBTYPE_SPLINE) {
            reqs.op_stack_len++; // Extra return frame
        }
        break;
    case OP_FIND_TOP_SURFACE:
        reqs.const_args_len += 2;
        reqs.op_stack_len++;
        break;
    case OP_RANGE_CHOICE:
        if (root->allChildren[0]->operationType != OP_CONST && root->allChildren[1]->operationType != OP_CONST)
            reqs.op_stack_len++;
        reqs.const_args_len += 3;
        break;
    case OP_INTERVAL_SELECT:
        reqs.const_args_len += (root->values[0] * 2) + 1;
        reqs.op_stack_len += root->values[0];
        break;
    case OP_SHIFTED_NOISE:
        reqs.const_args_len += 3;
        break;
    default:
        break;
    }
    // Add requirements of all children to current op reqs
    for (int i = 0; i < root->numChildren; i++) {
        if (root->allChildren[i]) {
            EvalStackAllocReqs childReqs = preorder_traversal_alloc_reqs(root->allChildren[i]);
            reqs.op_stack_len += childReqs.op_stack_len;
            reqs.const_args_len += childReqs.const_args_len;
        }
    }

    return reqs;
}

static int generate_stack_recursive(EvalStack* evalStack, Node* node, int regPos) {
    if (node->operationType == OP_CONST) {
        // Add value to const_args and increment counter
        evalStack->const_args[evalStack->const_args_len++] = node->values[0];
        return -evalStack->const_args_len;
    }
    // Keep running count of maximum register for later use
    evalStack->max_reg = (regPos > evalStack->max_reg) ? regPos : evalStack->max_reg;

    // Note: Stack frame used to be allocated but now is instead stored directly on the op stack.
    // Therefore we need the index of this frame for changing it's arg_indexes later.
    size_t currentFrameIndex = evalStack->op_stack_len;

    // Create new frame on stack and increment counter
    evalStack->op_stack[evalStack->op_stack_len++] = (StackFrame){
        .op_type = node->operationType,
        .result_index = regPos
    };
    // Execute type-based stack generation
    return node->generate_stack_recursive_from_op(evalStack, currentFrameIndex, node, regPos);
}

EvalStack* generate_stack_wrapper(Node* node) {
    preorder_traversal_depth_count(node);
    EvalStackAllocReqs evalStackAlloc = preorder_traversal_alloc_reqs(node);

    EvalStack* evalStack = (EvalStack*)malloc(sizeof(EvalStack));
    // Allocate stack arrays
    evalStack->op_stack = (StackFrame*)malloc(sizeof(StackFrame) * (evalStackAlloc.op_stack_len + 1));
    evalStack->op_stack[0] = (StackFrame){.op_type = OP_HALT};
    evalStack->op_stack_len = 1;
    evalStack->const_args = (double*)malloc(sizeof(double) * evalStackAlloc.const_args_len);
    evalStack->const_args_len = 0;
    evalStack->max_reg = 0;

    generate_stack_recursive(evalStack, node, 0);
    //printf("alloc reqs %d %d\n", evalStackAlloc.op_stack_len, evalStackAlloc.const_args_len);
    //printf("real reqs %d %d\n", evalStack->op_stack_len, evalStack->const_args_len);

    return evalStack;
}

static int spline_op_gen(EvalStack* evalStack, size_t currentFrameIndex, Node* node, int regPos) {
    evalStack->op_stack[currentFrameIndex].const_index = -(currentFrameIndex + 1);
    int num_points = node->values[1];
    double constants[num_points * 3];
    int pointFrameIndexes[num_points];

    for (int i = 0; i < num_points; i++) {
        Node* point = node->allChildren[i];
        size_t pointFrameIndex = evalStack->op_stack_len; // Save reference to current point StackFrame
        evalStack->op_stack[evalStack->op_stack_len++] = (StackFrame){ .op_type = point->operationType, .result_index = regPos };
        constants[i*3] = point->values[1]; // Add point location
        constants[(i*3) + 1] = point->values[2]; // Add point derivative
        int index = generate_stack_recursive(evalStack, point->allChildren[0], regPos + 3); // Generate point value stack

        // If point value is constant, store index as negative const_args index, else store op_stack index for evaluating point
        if (index < 0) {
            pointFrameIndexes[i] = -1;
            // Undo spline point add
            evalStack->op_stack_len--;
            constants[(i*3) + 2] = (double)index;
        } else {
            pointFrameIndexes[i] = pointFrameIndex;
            constants[(i*3) + 2] = evalStack->op_stack_len;
            evalStack->op_stack[pointFrameIndex].arg_indexes[1] = i;
            // Store return frame index as negative const_index
            evalStack->op_stack[pointFrameIndex].const_index = -(currentFrameIndex + 1);
        }
    }
    int main_spline_index = evalStack->op_stack_len;
    evalStack->op_stack[evalStack->op_stack_len++] = (StackFrame){ .op_type = node->operationType, .result_index = regPos };
    evalStack->op_stack[currentFrameIndex].arg_indexes[0] = main_spline_index;

    for (int i = 0; i < num_points; i++) {
        int index = pointFrameIndexes[i];
        if (index != -1) {
            // Store main spline frame in first arg index of points
            evalStack->op_stack[index].arg_indexes[0] = main_spline_index;
        }
    }

    // Add constants to const_args stack
    evalStack->op_stack[main_spline_index].const_index = evalStack->const_args_len;
    for (int i = 0; i < num_points * 3; i++) {
        evalStack->const_args[evalStack->const_args_len++] = constants[i];
    }
    // Evaluate coordinate first on stack
    if (node->allChildren[num_points]) {
        evalStack->op_stack[main_spline_index].arg_indexes[0] = generate_stack_recursive(evalStack, node->allChildren[num_points], regPos);
    }

    // Store return frame index as first arg
    evalStack->op_stack[main_spline_index].arg_indexes[1] = currentFrameIndex;
    evalStack->op_stack[main_spline_index].arg_indexes[2] = num_points;
    return regPos;
}

static int y_clamp_op_gen(EvalStack* evalStack, size_t currentFrameIndex, Node* node, int regPos) {
    evalStack->op_stack[currentFrameIndex].const_index = evalStack->const_args_len;
    for (int i = 0; i < 4; i++)
        evalStack->const_args[evalStack->const_args_len++] = node->values[i];
    return regPos;
}

static int find_top_surface_op_gen(EvalStack* evalStack, size_t currentFrameIndex, Node* node, int regPos) {
    evalStack->op_stack[currentFrameIndex].const_index = evalStack->const_args_len;
    for (int i = 0; i < 2; i++)
        evalStack->const_args[evalStack->const_args_len++] = node->values[i];

    // Density arg
    evalStack->op_stack[currentFrameIndex].arg_indexes[1] = generate_stack_recursive(evalStack, node->allChildren[1], regPos + 1);
    // Loopback PC index for density arg
    evalStack->op_stack[currentFrameIndex].arg_indexes[2] = evalStack->op_stack_len;

    // Find top surface early return condition
    evalStack->op_stack[evalStack->op_stack_len++] = (StackFrame){ .op_type = OP_FIND_TOP_SURFACE, .const_index = -(currentFrameIndex + 1) };
    // Upper bound arg
    evalStack->op_stack[currentFrameIndex].arg_indexes[0] = generate_stack_recursive(evalStack, node->allChildren[0], regPos);

    return regPos;
}

static int range_choice_op_gen(EvalStack* evalStack, size_t currentFrameIndex, Node* node, int regPos) {
    evalStack->op_stack_len--; // Undo initial frame add
    // Generate when out of range branch
    int outOfRangeIndex = generate_stack_recursive(evalStack, node->allChildren[2], regPos);
    int outOfRangePCIndex = evalStack->op_stack_len;

    // Generate in range argument
    if (node->allChildren[0]->operationType != OP_CONST && node->allChildren[1]->operationType != OP_CONST)
        evalStack->op_stack[evalStack->op_stack_len++] = (StackFrame){ .op_type = OP_RANGE_CHOICE, .const_index = -(currentFrameIndex + 1) };
    int inRangeIndex = generate_stack_recursive(evalStack, node->allChildren[1], regPos);

    size_t mainFrameIndex = evalStack->op_stack_len;
    evalStack->op_stack[evalStack->op_stack_len++] = (StackFrame){ .op_type = OP_RANGE_CHOICE, .const_index = evalStack->const_args_len, .result_index = regPos };
    evalStack->const_args[evalStack->const_args_len++] = node->values[0]; // min inclusive
    evalStack->const_args[evalStack->const_args_len++] = node->values[1]; // max exclusive
    evalStack->const_args[evalStack->const_args_len++] = currentFrameIndex; // Return PC index
    // Input argument
    evalStack->op_stack[mainFrameIndex].arg_indexes[0] = generate_stack_recursive(evalStack, node->allChildren[0], regPos);
    evalStack->op_stack[mainFrameIndex].arg_indexes[1] = inRangeIndex;
    evalStack->op_stack[mainFrameIndex].arg_indexes[2] = outOfRangeIndex < 0 ? outOfRangeIndex : outOfRangePCIndex;

    return regPos;
}

static int interval_select_op_gen(EvalStack* evalStack, size_t currentFrameIndex, Node* node, int regPos) {
    evalStack->op_stack_len--; // Undo initial frame add
    int num_thresholds = node->values[0];
    int num_functions = num_thresholds + 1;
    int threshold_indexes[num_functions];
    for (int i = 0; i < num_functions; i++) {
        int threshold_frame_index = evalStack->op_stack_len;

        int threshold_index = generate_stack_recursive(evalStack, node->allChildren[i + 1], regPos); // Threshold functions
        if (threshold_index < 0) {
            evalStack->op_stack_len--; // Undo frame add if threshold value is constant
        } else if (threshold_index >= 0) {
            threshold_indexes[i] = evalStack->op_stack_len;
        }
        evalStack->op_stack[evalStack->op_stack_len++] = (StackFrame){ .op_type = OP_INTERVAL_SELECT, .const_index = -(currentFrameIndex + 1), .result_index = regPos };
    }

    size_t mainFrameIndex = evalStack->op_stack_len - 1;
    // Replace last frame
    evalStack->op_stack[mainFrameIndex] = (StackFrame){ .op_type = OP_INTERVAL_SELECT, .const_index = evalStack->const_args_len, .result_index = regPos };
    for (int i = 0; i < num_thresholds; i++) {
        evalStack->const_args[evalStack->const_args_len++] = node->values[i + 1]; // threshold locations
    }
    for (int i = 0; i < num_functions; i++) {
        evalStack->const_args[evalStack->const_args_len++] = threshold_indexes[i];
    }

    // Input value
    evalStack->op_stack[mainFrameIndex].arg_indexes[0] = generate_stack_recursive(evalStack, node->allChildren[0], regPos);
    evalStack->op_stack[mainFrameIndex].arg_indexes[1] = num_thresholds;
    evalStack->op_stack[mainFrameIndex].arg_indexes[2] = currentFrameIndex;

    return regPos;
}

static int noise_op_gen(EvalStack* evalStack, size_t currentFrameIndex, Node* node, int regPos) {
    evalStack->op_stack[currentFrameIndex].const_index = evalStack->const_args_len;
    evalStack->const_args[evalStack->const_args_len++] = node->values[0]; // Noise state index
    evalStack->const_args[evalStack->const_args_len++] = node->values[1]; // xz scale value
    evalStack->const_args[evalStack->const_args_len++] = node->values[2]; // y scale value
    return regPos;
}

static int shift_op_gen(EvalStack* evalStack, size_t currentFrameIndex, Node* node, int regPos) {
    evalStack->op_stack[currentFrameIndex].const_index = evalStack->const_args_len;
    evalStack->const_args[evalStack->const_args_len++] = node->values[0]; // Noise state index
    return regPos;
}

static int old_blended_noise_op_gen(EvalStack* evalStack, size_t currentFrameIndex, Node* node, int regPos) {
    evalStack->op_stack[currentFrameIndex].const_index = evalStack->const_args_len;
    for (int i = 0; i < 5; i++) {
        evalStack->const_args[evalStack->const_args_len++] = node->values[i];
    }
    return regPos;
}

static int one_arg_op_gen(EvalStack* evalStack, size_t currentFrameIndex, Node* node, int regPos) {
    if (node->allChildren[0]) {
        evalStack->op_stack[currentFrameIndex].arg_indexes[0] = generate_stack_recursive(evalStack, node->allChildren[0], regPos);
    }
    return regPos;
}

static int two_args_op_gen(EvalStack* evalStack, size_t currentFrameIndex, Node* node, int regPos) {
    if (node->allChildren[0] && node->allChildren[1]) {
        bool switchArgs = node->allChildren[0]->depth > node->allChildren[1]->depth;
        evalStack->op_stack[currentFrameIndex].arg_indexes[switchArgs] = generate_stack_recursive(evalStack, switchArgs ? node->allChildren[1] : node->allChildren[0], regPos + 1);
        evalStack->op_stack[currentFrameIndex].arg_indexes[!switchArgs] = generate_stack_recursive(evalStack, switchArgs ? node->allChildren[0] : node->allChildren[1], regPos);
    }
    return regPos;
}

static int clamp_op_gen(EvalStack* evalStack, size_t currentFrameIndex, Node* node, int regPos) {
    evalStack->op_stack[currentFrameIndex].const_index = evalStack->const_args_len;
    evalStack->const_args[evalStack->const_args_len++] = node->values[0]; // min value
    evalStack->const_args[evalStack->const_args_len++] = node->values[1]; // max value
    return one_arg_op_gen(evalStack, currentFrameIndex, node, regPos);
}

static int shifted_noise_op_gen(EvalStack* evalStack, size_t currentFrameIndex, Node* node, int regPos) {
    evalStack->op_stack[currentFrameIndex].const_index = evalStack->const_args_len;
    evalStack->const_args[evalStack->const_args_len++] = node->values[0]; // noise index
    evalStack->const_args[evalStack->const_args_len++] = node->values[1]; // xz_scale
    evalStack->const_args[evalStack->const_args_len++] = node->values[2]; // y_scale

    // ISSUE: Interpreter must know which arg is x, y, and z.
    // Re-arranging arg positions to reduce registers causes this issue.
    // Either a indicator must be added to identify which arg is which or
    // we just don't re-arrange since it doesn't seem to reduce reg usage.
    /*
    Node* child[3];
    for (int i = 0; i < 3; i++)
        child[i] = node->allChildren[i];

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2 - i; j++) {
            if (child[j]->depth > child[j + 1]->depth) {
                Node* temp = child[j + 1];
                child[j + 1] = child[j];
                child[j] = temp;
            }
        }
    }

    for (int i = 0; i < 3; i++)
        evalStack->op_stack[currentFrameIndex].arg_indexes[i] = generate_stack_recursive(evalStack, node->allChildren[i], regPos + (2 - i));
    */
    int child_reg = regPos;
    for (int i = 0; i < 3; i++) {
        size_t index = generate_stack_recursive(evalStack, node->allChildren[i], child_reg);
        evalStack->op_stack[currentFrameIndex].arg_indexes[i] = index;
        // If child was a operation, increment child register position
        if (index >= 0) {
            child_reg++;
        }
    }

    return regPos;
}

Node* create_leaf(int op_type, double* theData) {
    Node* n          = (Node*)malloc(sizeof(Node));
    n->type          = NODE_LEAF;
    n->operationType = op_type;
    n->depth         = 0;
    n->label         = 0;
    n->numChildren   = 0;
    switch (op_type)
    {
    case OP_CONST:
        n->values = (double*)malloc(sizeof(double));
        memcpy(n->values, theData, sizeof(double));
        break;
    case OP_Y_CLAMPED_GRAD:
        n->generate_stack_recursive_from_op = y_clamp_op_gen;
        n->values = (double*)malloc(4 * sizeof(double));
        memcpy(n->values, theData, 4 * sizeof(double));
        break;
    case OP_NOISE:
        n->generate_stack_recursive_from_op = noise_op_gen;
        n->values = (double*)malloc(3 * sizeof(double));
        memcpy(n->values, theData, 3 * sizeof(double));
        break;
    case OP_SHIFT_A:
    case OP_SHIFT_B:
        n->generate_stack_recursive_from_op = shift_op_gen;
        n->values = (double*)malloc(sizeof(double));
        n->values[0] = theData[0];
        break;
    case OP_OLD_BLENDED_NOISE:
        n->generate_stack_recursive_from_op = old_blended_noise_op_gen;
        n->values = (double*)malloc(5 * sizeof(double));
        memcpy(n->values, theData, 5 * sizeof(double));
        break;
    case OP_BLEND_OFFSET:
        n->operationType = OP_CONST;
        n->values = (double*)malloc(sizeof(double));
        n->values[0] = 0.0;
        break;
    case OP_BLEND_ALPHA:
    case OP_END_ISLANDS:
        n->operationType = OP_CONST;
        n->values = (double*)malloc(sizeof(double));
        n->values[0] = 1.0;
        break;
    default:
        break;
    }
    return n;
}

Node* create_op(int op_type, Node** theChildren, double* theData) {
    Node* n    = (Node*)malloc(sizeof(Node));
    n->type    = NODE_OP;
    n->operationType = op_type;
    n->depth = 0;
    n->label   = 0;

    switch (op_type)
    {
    case OP_ADD:
    case OP_MUL:
    case OP_MIN:
    case OP_MAX:
        n->generate_stack_recursive_from_op = two_args_op_gen;
        n->allChildren = (Node**)malloc(2 * sizeof(Node*));
        n->allChildren[0] = theChildren[0];
        n->allChildren[1] = theChildren[1];
        n->numChildren = 2;
        break;
    case OP_FIND_TOP_SURFACE:
        n->generate_stack_recursive_from_op = find_top_surface_op_gen;
        n->allChildren = (Node**)malloc(2 * sizeof(Node*));
        n->allChildren[0] = theChildren[0]; // upper bound
        n->allChildren[1] = theChildren[1]; // density
        n->numChildren = 2;
        n->values = (double*)malloc(2 * sizeof(double));
        n->values[0] = theData[0]; // cell height
        n->values[1] = theData[1]; // lower bound
        break;
    case OP_RANGE_CHOICE:
        n->generate_stack_recursive_from_op = range_choice_op_gen;
        n->allChildren = (Node**)malloc(3 * sizeof(Node*));
        n->allChildren[0] = theChildren[0]; // input
        n->allChildren[1] = theChildren[1]; // when in range
        n->allChildren[2] = theChildren[2]; // when out of range
        n->numChildren = 3;
        n->values = (double*)malloc(2 * sizeof(double));
        n->values[0] = theData[0]; // min inclusive
        n->values[1] = theData[1]; // max exclusive
        break;
    case OP_INTERVAL_SELECT:
        n->generate_stack_recursive_from_op = interval_select_op_gen;
        int num_thresholds = theData[0];
        int num_children = num_thresholds + 2;
        n->allChildren = (Node**)malloc(num_children * sizeof(Node*)); // input plus threshold functions
        memcpy(n->allChildren, theChildren, num_children * sizeof(Node*));
        n->numChildren = num_children;
        n->values = (double*)malloc((num_thresholds + 1) * sizeof(double)); // number of thresholds plus threshold locations
        memcpy(n->values, theData, (num_thresholds + 1) * sizeof(double));
        break;
    case OP_SHIFTED_NOISE:
        n->generate_stack_recursive_from_op = shifted_noise_op_gen;
        n->allChildren = (Node**)malloc(3 * sizeof(Node*));
        n->allChildren[0] = theChildren[0];
        n->allChildren[1] = theChildren[1];
        n->allChildren[2] = theChildren[2];
        n->numChildren = 3;
        n->values = (double*)malloc(3 * sizeof(double));
        n->values[0] = theData[0]; // noise index
        n->values[1] = theData[1]; // xz scale
        n->values[2] = theData[2]; // y scale
        break;
    case OP_ABS:
    case OP_SQUARE:
    case OP_CUBE:
    case OP_HALF_NEGATIVE:
    case OP_QUARTER_NEGATIVE:
    case OP_INVERT:
    case OP_SQUEEZE:
    case OP_BLEND_DENSITY:
    case OP_INTERPOLATED:
    case OP_CACHE_ONCE:
    case OP_FLAT_CACHE:
    case OP_CACHE_2D:
    case OP_CACHE_ALL_IN_CELL:
        n->generate_stack_recursive_from_op = one_arg_op_gen;
        n->allChildren = (Node**)malloc(sizeof(Node*));
        n->allChildren[0] = theChildren[0];
        n->numChildren = 1;
        break;
    case OP_CLAMP:
        n->generate_stack_recursive_from_op = clamp_op_gen;
        n->allChildren = (Node**)malloc(sizeof(Node*));
        n->allChildren[0] = theChildren[0];
        n->numChildren = 1;
        n->values = (double*)malloc(2 * sizeof(double));
        n->values[0] = theData[0];
        n->values[1] = theData[1];
        break;
    case OP_SPLINE:
        n->generate_stack_recursive_from_op = spline_op_gen;
        // Check if subtype is spline point or main spline
        if (theData[0] == OP_SUBTYPE_SPLINE_POINT) {
            // Arg (value)
            n->allChildren = (Node**)malloc(sizeof(Node*));
            n->allChildren[0] = theChildren[0];
            n->numChildren = 1;
            // Constants (deriv and location)
            n->values = (double*)malloc(3 * sizeof(double));
            memcpy(n->values, theData, 3 * sizeof(double));
        } else if (theData[0] == OP_SUBTYPE_SPLINE) {
            // theData[1] is the number of spline points in the children array
            int total_children = theData[1] + 1;
            n->allChildren = (Node**)malloc(total_children * sizeof(Node*));
            // spline points followed by coordinate arg
            memcpy(n->allChildren, theChildren, total_children * sizeof(Node*));
            n->numChildren = total_children;
            n->values = (double*)malloc(2 * sizeof(double));
            memcpy(n->values, theData, 2 * sizeof(double));
        }
        break;
    default:
        n->generate_stack_recursive_from_op = two_args_op_gen;
        break;
    }

    return n;
}

int rotation_is_safe(Node* rest) {
    // rest        = the node currently sitting at tail->right
    // left_child  = rest->left  (the node we want to hoist)

    if (!rest || rest->type == NODE_LEAF) return 0;
    Node* left_child = rest->allChildren[0];
    if (!left_child || left_child->type == NODE_LEAF) return 0;

    OpInfo* op_rest  = lookup_op(rest->operationType);
    OpInfo* op_left  = lookup_op(left_child->operationType);
    if (!op_rest || !op_left) return 0;     // unknown operators → be conservative

    // The left child must bind no more tightly than its parent.
    // If it binds MORE tightly, hoisting it would change precedence.
    if (op_left->priority < op_rest->priority) return 0;

    // If same priority, the operator must be associative.
    if (!op_left->associative) return 0;

    return 1;
}

void rotate_right(Node* tail) {
    Node* old_root   = tail->allChildren[1];
    Node* new_root   = old_root->allChildren[0];
    old_root->allChildren[0]   = new_root->allChildren[1];
    new_root->allChildren[1]  = old_root;
    tail->allChildren[1]      = new_root;
}

int larger_arg_right(Node* root) {
    OpInfo* op_info = lookup_op(root->operationType);
    if (root->numChildren != 2 || !(op_info && op_info->associative)) {
        if (root->numChildren == 0) {
            return 0;
        }
        int max_children = 0;
        for (int i = 0; i < root->numChildren; i++) {
            int arg_max_children = larger_arg_right(root->allChildren[i]);
            if (arg_max_children > max_children)
                max_children = arg_max_children;
        }
        return max_children;
    }

    int left_max_num_children = larger_arg_right(root->allChildren[0]);
    int right_max_num_children = larger_arg_right(root->allChildren[1]);

    if (left_max_num_children > right_max_num_children){
        Node* temp = root->allChildren[0];
        root->allChildren[0] = root->allChildren[1];
        root->allChildren[1] = temp;
    }

    return max((int []){left_max_num_children, right_max_num_children, root->numChildren}, 3);
}

Node* tree_to_vine(Node* root) {
    // Prevent bad child dereferences
    if (root->operationType == OP_CONST) return root;
    larger_arg_right(root);
    Node pseudo_root;
    pseudo_root.allChildren     = (Node*[2]){0};
    pseudo_root.allChildren[1]  = root;
    pseudo_root.allChildren[0]  = NULL;
    pseudo_root.type            = NODE_OP;
    pseudo_root.operationType   = 0;
    pseudo_root.label           = 0;

    Node* tail = &pseudo_root;
    Node* rest = tail->allChildren[1];
    if (!lookup_op(root->operationType)) {
        // Passthrough tree_to_vine
        for (int i = 0; i < root->numChildren; i++) {
            root->allChildren[i] = tree_to_vine(root->allChildren[i]);
        }
        return root;
    }

    while (rest != NULL) {
        if (rest->numChildren < 2) break; // Prevent bad child dereferences

        if (rotation_is_safe(rest)) {
            rotate_right(tail);
            rest = tail->allChildren[1];   // re-examine the newly promoted node
        } else {
            // Cannot rotate here — skip down the right spine
            tail = rest;
            rest = rest->allChildren[1];
        }
    }

    return pseudo_root.allChildren[1];
}

double get_arg(EvalStack* s, double* registers, int offset, int arg_num) {
    int pos = s->op_stack[offset].arg_indexes[arg_num];
    // if (pos < 0)
    //     printf("%lf\n", s->const_args[abs(pos) - 1]);
    return pos < 0 ? s->const_args[abs(pos) - 1] : registers[pos];
}

double get_const(EvalStack* s, int offset, int arg_num) {
    return s->const_args[s->op_stack[offset].const_index + arg_num];
}

double evaluate_stack(EvalStack* finalStack, int x, int y, int z) {

    //printf("Registers Used: %d\n", finalStack->max_reg+1);
    double regArray[finalStack->max_reg+1];

    /*for (int i = finalStack->op_stack_len; i >= 0;)
    {
        //printf("%d %d %d\n", finalStack->op_stack[i].op_type, finalStack->op_stack[i].result_index, i);
        //fflush(stdout);
        switch (finalStack->op_stack[--i].op_type)
        {
        case OP_ADD:
            regArray[finalStack->op_stack[i].result_index] = get_arg(finalStack, regArray, i, 0) + get_arg(finalStack, regArray, i, 1);
            break;
        case OP_MUL:
            regArray[finalStack->op_stack[i].result_index] = get_arg(finalStack, regArray, i, 0) * get_arg(finalStack, regArray, i, 1);
            break;
        case OP_MIN: {
            double arg1 = get_arg(finalStack, regArray, i, 0);
            double arg2 = get_arg(finalStack, regArray, i, 1);
            regArray[finalStack->op_stack[i].result_index] = arg1 < arg2 ? arg1 : arg2;
            break;
        }
        case OP_MAX: {
            double arg1 = get_arg(finalStack, regArray, i, 0);
            double arg2 = get_arg(finalStack, regArray, i, 1);
            regArray[finalStack->op_stack[i].result_index] = arg1 > arg2 ? arg1 : arg2;
            break;
        }
        case OP_ABS:
            regArray[finalStack->op_stack[i].result_index] = fabs(get_arg(finalStack, regArray, i, 0));
            break;
        case OP_SQUARE: {
            double arg1 = get_arg(finalStack, regArray, i, 0);
            regArray[finalStack->op_stack[i].result_index] = arg1 * arg1;
            break;
        }
        case OP_CUBE: {
            double arg1 = get_arg(finalStack, regArray, i, 0);
            regArray[finalStack->op_stack[i].result_index] = arg1 * arg1 * arg1;
            break;
        }
        case OP_HALF_NEGATIVE: {
            double arg1 = get_arg(finalStack, regArray, i, 0);
            regArray[finalStack->op_stack[i].result_index] = arg1 < 0.0 ? arg1 * 0.5 : arg1;
            break;
        }
        case OP_INVERT:
            regArray[finalStack->op_stack[i].result_index] = 1.0 / get_arg(finalStack, regArray, i, 0);
            break;
        case OP_QUARTER_NEGATIVE: {
            double arg1 = get_arg(finalStack, regArray, i, 0);
            regArray[finalStack->op_stack[i].result_index] = arg1 < 0.0 ? arg1 * 0.25 : arg1;
            break;
        }
        case OP_SQUEEZE:
            double clamped_val = fmin(fmax(get_arg(finalStack, regArray, i, 0), -1.0), 1.0);
            regArray[finalStack->op_stack[i].result_index] = (clamped_val / 2.0) - ((clamped_val * clamped_val * clamped_val) / 24.0);
            break;
        case OP_CLAMP:
            double minimum = get_const(finalStack, i, 0);
            double maximum = get_const(finalStack, i, 1);
            regArray[finalStack->op_stack[i].result_index] = fmin(fmax(get_arg(finalStack, regArray, i, 0), minimum), maximum);
            break;
        case OP_Y_CLAMPED_GRAD:
            double fromY = get_const(finalStack, i, 0);
            double toY = get_const(finalStack, i, 1);
            double fromValue = get_const(finalStack, i, 2);
            double toValue = get_const(finalStack, i, 3);
            regArray[finalStack->op_stack[i].result_index] = clamped_map((double)y, fromY, toY, fromValue, toValue);
            break;
        case OP_FIND_TOP_SURFACE:
            if (finalStack->op_stack[i].const_index < 0) {
                // Find Top Surface Early Return Condition
                int main_pc_index = abs(finalStack->op_stack[i].const_index) - 1;
                int cell_height = (int)get_const(finalStack, main_pc_index, 0);
                int lower_bound = (int)get_const(finalStack, main_pc_index, 1);
                int top_y = ((int)floor(get_arg(finalStack, regArray, main_pc_index, 0) / cell_height)) * cell_height;
                if (top_y <= lower_bound) {
                    regArray[finalStack->op_stack[i].result_index] = lower_bound;
                    i = main_pc_index;
                } else {
                    // Save current y and switch y context to top_y
                    regArray[finalStack->op_stack[i].result_index] = (double)y;
                    y = top_y;
                }
            } else {
                // Find Top Surface Main
                int cell_height = (int)get_const(finalStack, i, 0);
                int lower_bound = (int)get_const(finalStack, i, 1);
                double density = get_arg(finalStack, regArray, i, 1);

                if (density > 0.0) {
                    int block_y = y;
                    // Switch y context back to old y
                    y = regArray[finalStack->op_stack[i].result_index];
                    regArray[finalStack->op_stack[i].result_index] = block_y;
                } else if (y >= lower_bound) {
                    if (finalStack->op_stack[i].arg_indexes[1] < 0) {
                        regArray[finalStack->op_stack[i].result_index + 1] = density;
                        i++;
                    } else {
                        i = finalStack->op_stack[i].arg_indexes[2];
                    }
                    y -= cell_height;
                } else {
                    // Switch y context back to old y
                    y = regArray[finalStack->op_stack[i].result_index];

                    regArray[finalStack->op_stack[i].result_index] = lower_bound;
                }
            }
            break;
        case OP_RANGE_CHOICE:
            if (finalStack->op_stack[i].const_index >= 0) {
                double input = get_arg(finalStack, regArray, i, 0);
                double min_incl = get_const(finalStack, i, 0);
                double max_excl = get_const(finalStack, i, 1);
                //printf("input %lf min %lf max %lf\n", input, min_incl, max_excl);
                //fflush(stdout);
                if (input < max_excl && input >= min_incl) {
                    int inRangeIndex = finalStack->op_stack[i].arg_indexes[1];
                    if (inRangeIndex < 0) {
                        regArray[finalStack->op_stack[i].result_index] = get_arg(finalStack, regArray, i, 1);
                        i = get_const(finalStack, i, 2);
                    }
                } else {
                    int outOfRangeIndex = finalStack->op_stack[i].arg_indexes[2];
                    if (outOfRangeIndex < 0) {
                        regArray[finalStack->op_stack[i].result_index] = get_arg(finalStack, regArray, i, 2);
                        i = get_const(finalStack, i, 2);
                    } else {
                        i = outOfRangeIndex;
                    }
                }
            } else {
                i = -(finalStack->op_stack[i].const_index + 1);
            }
            break;
        case OP_INTERVAL_SELECT:
            if (finalStack->op_stack[i].const_index >= 0) {
                int num_thresholds = finalStack->op_stack[i].arg_indexes[1];
                double input = get_arg(finalStack, regArray, i, 0);
                // Find interval range
                for (int j = 0; j <= num_thresholds; j++) {
                    if (j == num_thresholds || input < get_const(finalStack, i, j)) {
                        int threshold_index = get_const(finalStack, i, num_thresholds + j);
                        if (threshold_index < 0) {
                            regArray[finalStack->op_stack[i].result_index] = finalStack->const_args[(-threshold_index) - 1];
                            i = finalStack->op_stack[i].arg_indexes[2];
                        } else {
                            i = threshold_index;
                        }
                        break;
                    }
                }
            } else {
                i = -(finalStack->op_stack[i].const_index + 1);
            }
            break;
        case OP_NOISE: {
            int noise_index = get_const(finalStack, i, 0);
            double xz_scale = get_const(finalStack, i, 1);
            double y_scale = get_const(finalStack, i, 2);
            regArray[finalStack->op_stack[i].result_index] = generate_normal_noise(
                &finalStack->noise_states[noise_index],
                finalStack->noise_levels,
                (double)x * xz_scale,
                (double)y * y_scale,
                (double)z * xz_scale
            );
            break;
        }
        case OP_SHIFTED_NOISE: {
            int noise_index = get_const(finalStack, i, 0);
            double xz_scale = get_const(finalStack, i, 1);
            double y_scale = get_const(finalStack, i, 2);
            double shifted_x = ((double)x * xz_scale) + get_arg(finalStack, regArray, i, 0);
            double shifted_y = ((double)y * y_scale) + get_arg(finalStack, regArray, i, 1);
            double shifted_z = ((double)z * xz_scale) + get_arg(finalStack, regArray, i, 2);
            regArray[finalStack->op_stack[i].result_index] = generate_normal_noise(
                &finalStack->noise_states[noise_index],
                finalStack->noise_levels,
                shifted_x,
                shifted_y,
                shifted_z
            );
            //printf("shift noise %lf %lf %lf\n", xz_scale, y_scale, regArray[finalStack->op_stack[i].result_index]);
            //fflush(stdout);
            break;
        }
        case OP_SPLINE:
            // If const_index is non negative, treat as main spline, else treat as spline point
            if (finalStack->op_stack[i].const_index >= 0) {
                double coord = get_arg(finalStack, regArray, i, 0);
                int num_points = finalStack->op_stack[i].arg_indexes[2];
                int len = num_points;
                // Search for point
                int point_index = 0;
                while (len > 0) {
                    int half = len / 2;
                    int mid = point_index + half;
                    if (coord < get_const(finalStack, i, mid * 3)) {
                        len = half;
                    } else {
                        point_index = mid + 1;
                        len -= half + 1;
                    }
                }
                point_index -= 1;

                if (point_index < 0 || point_index == num_points - 1) {
                    point_index *= 3; // Convert to 3 value indexing
                    // If first point or last point (single point)
                    if (point_index < 0)
                        point_index = 0;
                    int point_PC = get_const(finalStack, i, point_index + 2);
                    // If first/last point is constant (negative), save result in register and go to return frame,
                    // else go to point value start frame
                    if (point_PC < 0) {
                        double point_loc = get_const(finalStack, i, point_index);
                        double point_deriv = get_const(finalStack, i, point_index + 1);
                        double point_value = finalStack->const_args[abs(point_PC) - 1];

                        // Calculate final value and set stack pos to return spline frame
                        regArray[finalStack->op_stack[i].result_index] = linear_ext_if_non_zero(point_deriv, point_loc, coord, point_value);
                        //printf("spline const %lf\n", regArray[finalStack->op_stack[i].result_index]);
                        //fflush(stdout);
                        i = finalStack->op_stack[i].arg_indexes[1];
                    } else {
                        // Set spline state in registers and set stack offset to compute first or last point
                        regArray[finalStack->op_stack[i].result_index + 1] = 0.0; // Single point indicator
                        i = point_PC;
                    }
                } else {
                    point_index *= 3; // Convert to 3 value indexing
                    // If between points, interpolate (double point)
                    int point1_PC = get_const(finalStack, i, point_index + 2);
                    int point2_PC = get_const(finalStack, i, point_index + 5);
                    // If both points are constants (negative), compute spline and go to return frame,
                    // else if point2 is constant (negative), start on first point
                    // else go to point2 start frame
                    if (point2_PC < 0 && point1_PC < 0) {
                        double point1_loc = get_const(finalStack, i, point_index);
                        double point1_deriv = get_const(finalStack, i, point_index + 1);
                        double point1_val = finalStack->const_args[abs(point1_PC) - 1];
                        double point2_loc = get_const(finalStack, i, point_index + 3);
                        double point2_deriv = get_const(finalStack, i, point_index + 4);
                        double point2_val = finalStack->const_args[abs(point2_PC) - 1];

                        regArray[finalStack->op_stack[i].result_index] = spline_compute(point1_loc, point1_deriv, point1_val, point2_loc, point2_deriv, point2_val, coord);
                        //printf("spline double const %lf\n", regArray[finalStack->op_stack[i].result_index]);
                        //fflush(stdout);
                        // Store final value and set stack offset to return frame
                        i = finalStack->op_stack[i].arg_indexes[1];
                    } else if (point2_PC < 0) {
                        regArray[finalStack->op_stack[i].result_index + 1] = 1.0; // First point eval indicator
                        i = point1_PC;
                    } else if (point1_PC < 0) {
                        regArray[finalStack->op_stack[i].result_index + 1] = 2.0; // Second point eval indicator
                        i = point2_PC;
                    } else {
                        regArray[finalStack->op_stack[i].result_index + 1] = 4.0; // Double point eval indicator
                        i = point2_PC;
                    }
                }
            } else {
                //printf("spline point\n");
                //fflush(stdout);
                // Spline point case
                int main_spline_reg_index = finalStack->op_stack[i].result_index;
                int main_spline_stack_index = finalStack->op_stack[i].arg_indexes[0];
                if (regArray[main_spline_reg_index + 1] == 0.0) {
                    // Single point eval case
                    int point_index = finalStack->op_stack[i].arg_indexes[1] * 3;
                    double coord = get_arg(finalStack, regArray, main_spline_stack_index, 0);
                    double point_loc = get_const(finalStack, main_spline_stack_index, point_index);
                    double point_deriv = get_const(finalStack, main_spline_stack_index, point_index + 1);
                    double point_value = regArray[main_spline_reg_index + 3];
                    regArray[finalStack->op_stack[i].result_index] = linear_ext_if_non_zero(point_deriv, point_loc, coord, point_value);
                    //printf("spline single arg %lf\n", regArray[finalStack->op_stack[i].result_index]);
                    //fflush(stdout);
                    i = -(finalStack->op_stack[i].const_index + 1);
                } else if (regArray[main_spline_reg_index + 1] >= 1.0 && regArray[main_spline_reg_index + 1] <= 3.0) {
                    // First point eval case, Second point eval case, and double point phase 2 eval case
                    int point_index = finalStack->op_stack[i].arg_indexes[1] * 3;
                    double coord = get_arg(finalStack, regArray, main_spline_stack_index, 0);
                    double point1_loc = get_const(finalStack, main_spline_stack_index, point_index);
                    double point1_deriv = get_const(finalStack, main_spline_stack_index, point_index + 1);
                    double point1_value = regArray[main_spline_reg_index + 1] != 2.0
                        ? regArray[main_spline_reg_index + 3] // First or double point eval
                        : finalStack->const_args[abs(get_const(finalStack, main_spline_stack_index, point_index + 2)) - 1]; // Second point eval
                    double point2_loc = get_const(finalStack, main_spline_stack_index, point_index + 3);
                    double point2_deriv = get_const(finalStack, main_spline_stack_index, point_index + 4);
                    double point2_value = regArray[main_spline_reg_index + 1] == 1.0
                        ? finalStack->const_args[abs(get_const(finalStack, main_spline_stack_index, point_index + 5)) - 1] // First point eval
                        : regArray[main_spline_reg_index + 1] == 2.0
                        ? regArray[main_spline_reg_index + 3] // Second point eval
                        : regArray[main_spline_reg_index + 2]; // Double point eval
                    regArray[finalStack->op_stack[i].result_index] = spline_compute(point1_loc, point1_deriv, point1_value, point2_loc, point2_deriv, point2_value, coord);
                    //printf("fp %lf sp %lf info %lf %lf\n", point1_value, point2_value, point2_loc, point2_deriv);
                    //printf("spline double arg %lf\n", regArray[finalStack->op_stack[i].result_index]);
                    //fflush(stdout);
                    i = -(finalStack->op_stack[i].const_index + 1);
                } else if (regArray[main_spline_reg_index + 1] == 4.0) {
                    // Double point eval case
                    regArray[main_spline_reg_index + 1]--; // Indicate phase 2 of double point eval
                    // Move point 2 value to prevent it from being overwritten
                    regArray[main_spline_reg_index + 2] = regArray[main_spline_reg_index + 3];
                }
            }
            break;
        case OP_BLEND_DENSITY:
        case OP_INTERPOLATED:
        case OP_CACHE_ONCE:
        case OP_FLAT_CACHE:
        case OP_CACHE_2D:
        case OP_CACHE_ALL_IN_CELL:
            regArray[finalStack->op_stack[i].result_index] = get_arg(finalStack, regArray, i, 0);
            break;
        case OP_SHIFT_A: {
            int noise_index = get_const(finalStack, i, 0);
            regArray[finalStack->op_stack[i].result_index] = 4.0 * generate_normal_noise(
                &finalStack->noise_states[noise_index],
                finalStack->noise_levels,
                (double)x * 0.25,
                0.0,
                (double)z * 0.25
            );
            break;
        }
        case OP_SHIFT_B: {
            int noise_index = get_const(finalStack, i, 0);
            regArray[finalStack->op_stack[i].result_index] = 4.0 * generate_normal_noise(
                &finalStack->noise_states[noise_index],
                finalStack->noise_levels,
                (double)z * 0.25,
                (double)x * 0.25,
                0.0
            );
            break;
        }
        case OP_OLD_BLENDED_NOISE: {
            double smear_scale_mult = get_const(finalStack, i, 0);
            double xz_factor = get_const(finalStack, i, 1);
            double xz_scale = get_const(finalStack, i, 2) * 684.412;
            double y_factor = get_const(finalStack, i, 3);
            double y_scale = get_const(finalStack, i, 4) * 684.412;
            double block_x_mul = (double)x * xz_scale;
            double block_y_mul = (double)y * y_scale;
            double block_z_mul = (double)z * xz_scale;
            double block_x_fact = block_x_mul / xz_factor;
            double block_y_fact = block_y_mul / y_factor;
            double block_z_fact = block_z_mul / xz_factor;
            double y_smear = y_scale * smear_scale_mult;
            double y_smear_factor = y_smear / y_factor;

            double noise_acc = 0.0;
            double acc = 1.0;
            for (int j = 0; j < 8; j++) {
                ImprovedNoise* noise_level = get_perlin_noise_level(
                    &finalStack->noise_states[0].noises[1],
                    finalStack->noise_levels,
                    j
                );
                noise_acc += generate_improved_noise(
                    noise_level,
                    wrap(block_x_fact * acc),
                    wrap(block_y_fact * acc),
                    wrap(block_z_fact * acc),
                    y_smear_factor * acc,
                    block_y_fact * acc
                ) / acc;
                acc /= 2.0;
            }

            double noise_result = (1.0 + (noise_acc / 10.0)) / 2.0;
            double min_noise_acc = 0.0;
            double max_noise_acc = 0.0;
            acc = 1.0;
            for (int j = 0; j < 16; j++) {
                double block_x_wrap = wrap(block_x_mul * acc);
                double block_y_wrap = wrap(block_y_mul * acc);
                double block_z_wrap = wrap(block_z_mul * acc);
                double y_smear_adj = y_smear * acc;
                double block_y_adj = block_y_mul * acc;
                ImprovedNoise* noise_limit = get_perlin_noise_level(
                    &finalStack->noise_states[0].noises[0],
                    finalStack->noise_levels,
                    j
                );
                if (!(noise_result <= 0.0)) {
                    max_noise_acc += generate_improved_noise(
                        noise_limit,
                        block_x_wrap,
                        block_y_wrap,
                        block_z_wrap,
                        y_smear_adj,
                        block_y_adj
                    ) / acc;
                }
                if (!(noise_result >= 1.0)) {
                    min_noise_acc += generate_improved_noise(
                        noise_limit + 16, // Assumes min/max perlin noise has 16 levels
                        block_x_wrap,
                        block_y_wrap,
                        block_z_wrap,
                        y_smear_adj,
                        block_y_adj
                    ) / acc;
                }
                acc /= 2.0;
            }
            double final_result;
            if (noise_result < 0.0) {
                final_result = min_noise_acc / (double)(1 << 16);
            } else if (noise_result > 1.0) {
                final_result = max_noise_acc / (double)(1 << 16);
            } else {
                final_result = lerp(noise_result, min_noise_acc / 512.0, max_noise_acc / 512.0) / 128.0;
            }

            //printf("old blend noise %lf %lf %lf %lf\n", noise_result, min_noise_acc, max_noise_acc, final_result);
            //fflush(stdout);
            regArray[finalStack->op_stack[i].result_index] = final_result;
            break;
        }
        default:
            break;
        }
        //for (int j = 0; j < finalStack->max_reg+1; j++)
        //    printf("%lf ", regArray[j]);
        //printf("%d %d\n", finalStack->op_stack[i].op_type, i);
        //fflush(stdout);
    }*/

    void* dispatch_table[] = {
        &&op_fallback, // OP_CONSTANT should not be present
        &&op_add,
        &&op_mul,
        &&op_min,
        &&op_max,
        &&op_abs,
        &&op_square,
        &&op_cube,
        &&op_half_negative,
        &&op_quarter_negative,
        &&op_invert,
        &&op_squeeze,
        &&op_clamp,
        &&op_y_clamped_grad,
        &&op_find_top_surface,
        &&op_range_choice,
        &&op_interval_select,
        &&op_noise,
        &&op_shifted_noise,
        &&op_spline,
        &&op_blend_density,
        &&op_blend_offset,
        &&op_blend_alpha,
        &&op_interpolated,
        &&op_cache_once,
        &&op_flat_cache,
        &&op_cache_2d,
        &&op_cache_all_in_cell,
        &&op_shift_a,
        &&op_shift_b,
        &&op_old_blended_noise,
        &&op_end_islands,
        &&op_halt
    };

    int pc = finalStack->op_stack_len;
    goto *dispatch_table[finalStack->op_stack[--pc].op_type];

    op_fallback:
        goto *dispatch_table[finalStack->op_stack[--pc].op_type];

    op_add:
        regArray[finalStack->op_stack[pc].result_index] = get_arg(finalStack, regArray, pc, 0) + get_arg(finalStack, regArray, pc, 1);
        goto *dispatch_table[finalStack->op_stack[--pc].op_type];

    op_mul:
        regArray[finalStack->op_stack[pc].result_index] = get_arg(finalStack, regArray, pc, 0) * get_arg(finalStack, regArray, pc, 1);
        goto *dispatch_table[finalStack->op_stack[--pc].op_type];

    op_min: {
        double arg1 = get_arg(finalStack, regArray, pc, 0);
        double arg2 = get_arg(finalStack, regArray, pc, 1);
        regArray[finalStack->op_stack[pc].result_index] = arg1 < arg2 ? arg1 : arg2;
        //printf("min arg1 %lf arg2 %lf\n", arg1, arg2);
        //fflush(stdout);
        goto *dispatch_table[finalStack->op_stack[--pc].op_type];
    }

    op_max: {
        double arg1 = get_arg(finalStack, regArray, pc, 0);
        double arg2 = get_arg(finalStack, regArray, pc, 1);
        regArray[finalStack->op_stack[pc].result_index] = arg1 > arg2 ? arg1 : arg2;
        goto *dispatch_table[finalStack->op_stack[--pc].op_type];
    }

    op_abs:
        regArray[finalStack->op_stack[pc].result_index] = fabs(get_arg(finalStack, regArray, pc, 0));
        goto *dispatch_table[finalStack->op_stack[--pc].op_type];

    op_square: {
        double arg1 = get_arg(finalStack, regArray, pc, 0);
        regArray[finalStack->op_stack[pc].result_index] = arg1 * arg1;
        goto *dispatch_table[finalStack->op_stack[--pc].op_type];
    }

    op_cube: {
        double arg1 = get_arg(finalStack, regArray, pc, 0);
        regArray[finalStack->op_stack[pc].result_index] = arg1 * arg1 * arg1;
        goto *dispatch_table[finalStack->op_stack[--pc].op_type];
    }

    op_half_negative: {
        double arg1 = get_arg(finalStack, regArray, pc, 0);
        regArray[finalStack->op_stack[pc].result_index] = arg1 < 0.0 ? arg1 * 0.5 : arg1;
        goto *dispatch_table[finalStack->op_stack[--pc].op_type];
    }

    op_quarter_negative: {
        double arg1 = get_arg(finalStack, regArray, pc, 0);
        regArray[finalStack->op_stack[pc].result_index] = arg1 < 0.0 ? arg1 * 0.25 : arg1;
        goto *dispatch_table[finalStack->op_stack[--pc].op_type];
    }

    op_invert:
        regArray[finalStack->op_stack[pc].result_index] = 1.0 / get_arg(finalStack, regArray, pc, 0);
        goto *dispatch_table[finalStack->op_stack[--pc].op_type];

    op_squeeze: {
        double clamped_val = fmin(fmax(get_arg(finalStack, regArray, pc, 0), -1.0), 1.0);
        regArray[finalStack->op_stack[pc].result_index] = (clamped_val / 2.0) - ((clamped_val * clamped_val * clamped_val) / 24.0);
        goto *dispatch_table[finalStack->op_stack[--pc].op_type];
    }

    op_clamp: {
        double minimum = get_const(finalStack, pc, 0);
        double maximum = get_const(finalStack, pc, 1);
        regArray[finalStack->op_stack[pc].result_index] = fmin(fmax(get_arg(finalStack, regArray, pc, 0), minimum), maximum);
        goto *dispatch_table[finalStack->op_stack[--pc].op_type];
    }

    op_y_clamped_grad: {
        double fromY = get_const(finalStack, pc, 0);
        double toY = get_const(finalStack, pc, 1);
        double fromValue = get_const(finalStack, pc, 2);
        double toValue = get_const(finalStack, pc, 3);
        regArray[finalStack->op_stack[pc].result_index] = clamped_map((double)y, fromY, toY, fromValue, toValue);
        goto *dispatch_table[finalStack->op_stack[--pc].op_type];
    }

    op_find_top_surface: {
        if (finalStack->op_stack[pc].const_index < 0) {
            // Find Top Surface Early Return Condition
            int main_pc_index = abs(finalStack->op_stack[pc].const_index) - 1;
            int cell_height = (int)get_const(finalStack, main_pc_index, 0);
            int lower_bound = (int)get_const(finalStack, main_pc_index, 1);
            int top_y = ((int)floor(get_arg(finalStack, regArray, main_pc_index, 0) / cell_height)) * cell_height;
            if (top_y <= lower_bound) {
                regArray[finalStack->op_stack[pc].result_index] = lower_bound;
                pc = main_pc_index;
            } else {
                // Save current y and switch y context to top_y
                regArray[finalStack->op_stack[pc].result_index] = (double)y;
                y = top_y;
            }
        } else {
            // Find Top Surface Main
            int cell_height = (int)get_const(finalStack, pc, 0);
            int lower_bound = (int)get_const(finalStack, pc, 1);
            double density = get_arg(finalStack, regArray, pc, 1);

            if (density > 0.0) {
                int block_y = y;
                // Switch y context back to old y
                y = regArray[finalStack->op_stack[pc].result_index];
                regArray[finalStack->op_stack[pc].result_index] = block_y;
            } else if (y >= lower_bound) {
                if (finalStack->op_stack[pc].arg_indexes[1] < 0) {
                    regArray[finalStack->op_stack[pc].result_index + 1] = density;
                    pc++;
                } else {
                    pc = finalStack->op_stack[pc].arg_indexes[2];
                }
                y -= cell_height;
            } else {
                // Switch y context back to old y
                y = regArray[finalStack->op_stack[pc].result_index];

                regArray[finalStack->op_stack[pc].result_index] = lower_bound;
            }
        }
        goto *dispatch_table[finalStack->op_stack[--pc].op_type];
    }

    op_range_choice: {
        if (finalStack->op_stack[pc].const_index >= 0) {
            double input = get_arg(finalStack, regArray, pc, 0);
            double min_incl = get_const(finalStack, pc, 0);
            double max_excl = get_const(finalStack, pc, 1);
            //printf("input %lf min %lf max %lf\n", input, min_incl, max_excl);
            //fflush(stdout);
            if (input < max_excl && input >= min_incl) {
                int inRangeIndex = finalStack->op_stack[pc].arg_indexes[1];
                if (inRangeIndex < 0) {
                    regArray[finalStack->op_stack[pc].result_index] = get_arg(finalStack, regArray, pc, 1);
                    pc = get_const(finalStack, pc, 2);
                }
            } else {
                int outOfRangeIndex = finalStack->op_stack[pc].arg_indexes[2];
                if (outOfRangeIndex < 0) {
                    regArray[finalStack->op_stack[pc].result_index] = get_arg(finalStack, regArray, pc, 2);
                    pc = get_const(finalStack, pc, 2);
                } else {
                    pc = outOfRangeIndex;
                }
            }
        } else {
            pc = -(finalStack->op_stack[pc].const_index + 1);
        }
        goto *dispatch_table[finalStack->op_stack[--pc].op_type];
    }

    op_interval_select: {
        if (finalStack->op_stack[pc].const_index >= 0) {
            int num_thresholds = finalStack->op_stack[pc].arg_indexes[1];
            double input = get_arg(finalStack, regArray, pc, 0);
            // Find interval range
            for (int j = 0; j <= num_thresholds; j++) {
                if (j == num_thresholds || input < get_const(finalStack, pc, j)) {
                    int threshold_index = get_const(finalStack, pc, num_thresholds + j);
                    if (threshold_index < 0) {
                        regArray[finalStack->op_stack[pc].result_index] = finalStack->const_args[(-threshold_index) - 1];
                        pc = finalStack->op_stack[pc].arg_indexes[2];
                    } else {
                        pc = threshold_index;
                    }
                    break;
                }
            }
        } else {
            pc = -(finalStack->op_stack[pc].const_index + 1);
        }
        goto *dispatch_table[finalStack->op_stack[--pc].op_type];
    }

    op_noise: {
        int noise_index = get_const(finalStack, pc, 0);
        double xz_scale = get_const(finalStack, pc, 1);
        double y_scale = get_const(finalStack, pc, 2);
        regArray[finalStack->op_stack[pc].result_index] = generate_normal_noise(
            &finalStack->noise_states[noise_index],
            finalStack->noise_levels,
            (double)x * xz_scale,
            (double)y * y_scale,
            (double)z * xz_scale
        );
        goto *dispatch_table[finalStack->op_stack[--pc].op_type];
    }

    op_shifted_noise: {
        int noise_index = get_const(finalStack, pc, 0);
        double xz_scale = get_const(finalStack, pc, 1);
        double y_scale = get_const(finalStack, pc, 2);
        double shifted_x = ((double)x * xz_scale) + get_arg(finalStack, regArray, pc, 0);
        double shifted_y = ((double)y * y_scale) + get_arg(finalStack, regArray, pc, 1);
        double shifted_z = ((double)z * xz_scale) + get_arg(finalStack, regArray, pc, 2);
        regArray[finalStack->op_stack[pc].result_index] = generate_normal_noise(
            &finalStack->noise_states[noise_index],
            finalStack->noise_levels,
            shifted_x,
            shifted_y,
            shifted_z
        );
        //printf("shift noise %lf %lf %lf\n", xz_scale, y_scale, regArray[finalStack->op_stack[pc].result_index]);
        //fflush(stdout);
        goto *dispatch_table[finalStack->op_stack[--pc].op_type];
    }

    op_spline: {
        // If const_index is non negative, treat as main spline, else treat as spline point
        if (finalStack->op_stack[pc].const_index >= 0) {
            double coord = get_arg(finalStack, regArray, pc, 0);
            int num_points = finalStack->op_stack[pc].arg_indexes[2];
            int len = num_points;
            // Search for point
            int point_index = 0;
            while (len > 0) {
                int half = len / 2;
                int mid = point_index + half;
                if (coord < get_const(finalStack, pc, mid * 3)) {
                    len = half;
                } else {
                    point_index = mid + 1;
                    len -= half + 1;
                }
            }
            point_index -= 1;
            //printf("pi %d num %d\n", point_index, num_points);
            //printf("loc %lf deriv %lf\n", get_const(finalStack, pc, point_index * 3), get_const(finalStack, pc, (point_index * 3) + 1));
            //fflush(stdout);

            if (point_index < 0 || point_index == num_points - 1) {
                // If first point or last point (single point)
                if (point_index < 0) {
                    point_index = 0; // Use first point
                } else {
                    point_index *= 3; // Convert to 3 value indexing
                }

                int point_PC = get_const(finalStack, pc, point_index + 2);
                // If first/last point is constant (negative), save result in register and go to return frame,
                // else go to point value start frame
                if (point_PC < 0) {
                    double point_loc = get_const(finalStack, pc, point_index);
                    double point_deriv = get_const(finalStack, pc, point_index + 1);
                    double point_value = finalStack->const_args[abs(point_PC) - 1];

                    // Calculate final value and set stack pos to return spline frame
                    regArray[finalStack->op_stack[pc].result_index] = linear_ext_if_non_zero(point_deriv, point_loc, coord, point_value);
                    //printf("spline const %lf\n", regArray[finalStack->op_stack[pc].result_index]);
                    //fflush(stdout);
                    pc = finalStack->op_stack[pc].arg_indexes[1];
                } else {
                    // Set spline state in registers and set stack offset to compute first or last point
                    regArray[finalStack->op_stack[pc].result_index + 1] = 0.0; // Single point indicator
                    pc = point_PC;
                }
            } else {
                point_index *= 3; // Convert to 3 value indexing
                // If between points, interpolate (double point)
                int point1_PC = get_const(finalStack, pc, point_index + 2);
                int point2_PC = get_const(finalStack, pc, point_index + 5);
                // If both points are constants (negative), compute spline and go to return frame,
                // else if point2 is constant (negative), start on first point
                // else go to point2 start frame
                if (point2_PC < 0 && point1_PC < 0) {
                    double point1_loc = get_const(finalStack, pc, point_index);
                    double point1_deriv = get_const(finalStack, pc, point_index + 1);
                    double point1_val = finalStack->const_args[abs(point1_PC) - 1];
                    double point2_loc = get_const(finalStack, pc, point_index + 3);
                    double point2_deriv = get_const(finalStack, pc, point_index + 4);
                    double point2_val = finalStack->const_args[abs(point2_PC) - 1];

                    regArray[finalStack->op_stack[pc].result_index] = spline_compute(point1_loc, point1_deriv, point1_val, point2_loc, point2_deriv, point2_val, coord);
                    //printf("spline double const %lf (loc1 %lf deriv1 %lf val1 %lf loc2 %lf deriv2 %lf val2 %lf coord %lf) pi %d\n", regArray[finalStack->op_stack[pc].result_index], point1_loc, point1_deriv, point1_val, point2_loc, point2_deriv, point2_val, coord, point_index);
                    //fflush(stdout);
                    // Store final value and set stack offset to return frame
                    pc = finalStack->op_stack[pc].arg_indexes[1];
                } else if (point2_PC < 0) {
                    regArray[finalStack->op_stack[pc].result_index + 1] = 1.0; // First point eval indicator
                    pc = point1_PC;
                } else if (point1_PC < 0) {
                    regArray[finalStack->op_stack[pc].result_index + 1] = 2.0; // Second point eval indicator
                    pc = point2_PC;
                } else {
                    regArray[finalStack->op_stack[pc].result_index + 1] = 4.0; // Double point eval indicator
                    pc = point2_PC;
                }
            }
        } else {
            //printf("spline point\n");
            //fflush(stdout);
            // Spline point case
            int main_spline_reg_index = finalStack->op_stack[pc].result_index;
            int main_spline_stack_index = finalStack->op_stack[pc].arg_indexes[0];
            if (regArray[main_spline_reg_index + 1] == 0.0) {
                // Single point eval case
                int point_index = finalStack->op_stack[pc].arg_indexes[1] * 3;
                double coord = get_arg(finalStack, regArray, main_spline_stack_index, 0);
                double point_loc = get_const(finalStack, main_spline_stack_index, point_index);
                double point_deriv = get_const(finalStack, main_spline_stack_index, point_index + 1);
                double point_value = regArray[main_spline_reg_index + 3];
                regArray[finalStack->op_stack[pc].result_index] = linear_ext_if_non_zero(point_deriv, point_loc, coord, point_value);
                //printf("spline single arg %lf\n", regArray[finalStack->op_stack[pc].result_index]);
                //fflush(stdout);
                pc = -(finalStack->op_stack[pc].const_index + 1);
            } else if (regArray[main_spline_reg_index + 1] >= 1.0 && regArray[main_spline_reg_index + 1] <= 3.0) {
                // First point eval case, Second point eval case, and double point phase 2 eval case
                int point_index = regArray[main_spline_reg_index + 1] == 2.0
                    ? (finalStack->op_stack[pc].arg_indexes[1] - 1) * 3
                    : finalStack->op_stack[pc].arg_indexes[1] * 3;
                double coord = get_arg(finalStack, regArray, main_spline_stack_index, 0);
                double point1_loc = get_const(finalStack, main_spline_stack_index, point_index);
                double point1_deriv = get_const(finalStack, main_spline_stack_index, point_index + 1);
                double point1_value = regArray[main_spline_reg_index + 1] != 2.0
                    ? regArray[main_spline_reg_index + 3] // First point eval
                    : finalStack->const_args[abs(get_const(finalStack, main_spline_stack_index, point_index + 2)) - 1]; // Second point eval
                double point2_loc = get_const(finalStack, main_spline_stack_index, point_index + 3);
                double point2_deriv = get_const(finalStack, main_spline_stack_index, point_index + 4);
                double point2_value = regArray[main_spline_reg_index + 1] == 1.0
                    ? finalStack->const_args[abs(get_const(finalStack, main_spline_stack_index, point_index + 5)) - 1] // First point eval
                    : regArray[main_spline_reg_index + 1] == 2.0
                    ? regArray[main_spline_reg_index + 3] // Second point eval
                    : regArray[main_spline_reg_index + 2]; // Double point eval
                regArray[finalStack->op_stack[pc].result_index] = spline_compute(point1_loc, point1_deriv, point1_value, point2_loc, point2_deriv, point2_value, coord);
                //printf("fp %lf sp %lf info %lf %lf %lf\n", point1_value, point2_value, point2_loc, point2_deriv, regArray[main_spline_reg_index + 1]);
                //printf("spline double arg %lf\n", regArray[finalStack->op_stack[pc].result_index]);
                //fflush(stdout);
                pc = -(finalStack->op_stack[pc].const_index + 1);
            } else if (regArray[main_spline_reg_index + 1] == 4.0) {
                // Double point eval case
                regArray[main_spline_reg_index + 1]--; // Indicate phase 2 of double point eval
                // Move point 2 value to prevent it from being overwritten
                regArray[main_spline_reg_index + 2] = regArray[main_spline_reg_index + 3];
                //int prev_point_index = (finalStack->op_stack[pc].arg_indexes[1] - 1) * 3;
                //pc = get_const(finalStack, main_spline_stack_index, prev_point_index + 2);
            }
        }
        goto *dispatch_table[finalStack->op_stack[--pc].op_type];
    }

    op_blend_density:
    op_interpolated:
    op_cache_once:
    op_flat_cache:
    op_cache_2d:
    op_cache_all_in_cell:
        regArray[finalStack->op_stack[pc].result_index] = get_arg(finalStack, regArray, pc, 0);
        goto *dispatch_table[finalStack->op_stack[--pc].op_type];

    op_blend_offset:
        regArray[finalStack->op_stack[pc].result_index] = 0.0;
        goto *dispatch_table[finalStack->op_stack[--pc].op_type];

    op_blend_alpha:
    op_end_islands:
        regArray[finalStack->op_stack[pc].result_index] = 1.0;
        goto *dispatch_table[finalStack->op_stack[--pc].op_type];

    op_shift_a: {
        int noise_index = get_const(finalStack, pc, 0);
        regArray[finalStack->op_stack[pc].result_index] = 4.0 * generate_normal_noise(
            &finalStack->noise_states[noise_index],
            finalStack->noise_levels,
            (double)x * 0.25,
            0.0,
            (double)z * 0.25
        );
        //printf("shift a %lf\n", regArray[finalStack->op_stack[pc].result_index]);
        //fflush(stdout);
        goto *dispatch_table[finalStack->op_stack[--pc].op_type];
    }

    op_shift_b: {
        int noise_index = get_const(finalStack, pc, 0);
        regArray[finalStack->op_stack[pc].result_index] = 4.0 * generate_normal_noise(
            &finalStack->noise_states[noise_index],
            finalStack->noise_levels,
            (double)z * 0.25,
            (double)x * 0.25,
            0.0
        );
        //printf("shift b %lf\n", regArray[finalStack->op_stack[pc].result_index]);
        //fflush(stdout);
        goto *dispatch_table[finalStack->op_stack[--pc].op_type];
    }

    op_old_blended_noise: {
        double smear_scale_mult = get_const(finalStack, pc, 0);
        double xz_factor = get_const(finalStack, pc, 1);
        double xz_scale = get_const(finalStack, pc, 2) * 684.412;
        double y_factor = get_const(finalStack, pc, 3);
        double y_scale = get_const(finalStack, pc, 4) * 684.412;
        double block_x_mul = (double)x * xz_scale;
        double block_y_mul = (double)y * y_scale;
        double block_z_mul = (double)z * xz_scale;
        double block_x_fact = block_x_mul / xz_factor;
        double block_y_fact = block_y_mul / y_factor;
        double block_z_fact = block_z_mul / xz_factor;
        double y_smear = y_scale * smear_scale_mult;
        double y_smear_factor = y_smear / y_factor;

        double noise_acc = 0.0;
        double acc = 1.0;
        for (int i = 0; i < 8; i++) {
            ImprovedNoise* noise_level = get_perlin_noise_level(
                &finalStack->noise_states[0].noises[1],
                finalStack->noise_levels,
                i
            );
            noise_acc += generate_improved_noise(
                noise_level,
                wrap(block_x_fact * acc),
                wrap(block_y_fact * acc),
                wrap(block_z_fact * acc),
                y_smear_factor * acc,
                block_y_fact * acc
            ) / acc;
            acc /= 2.0;
        }

        double noise_result = (1.0 + (noise_acc / 10.0)) / 2.0;
        double min_noise_acc = 0.0;
        double max_noise_acc = 0.0;
        acc = 1.0;
        for (int i = 0; i < 16; i++) {
            double block_x_wrap = wrap(block_x_mul * acc);
            double block_y_wrap = wrap(block_y_mul * acc);
            double block_z_wrap = wrap(block_z_mul * acc);
            double y_smear_adj = y_smear * acc;
            double block_y_adj = block_y_mul * acc;
            ImprovedNoise* noise_limit = get_perlin_noise_level(
                &finalStack->noise_states[0].noises[0],
                finalStack->noise_levels,
                i
            );
            if (!(noise_result <= 0.0)) {
                max_noise_acc += generate_improved_noise(
                    noise_limit,
                    block_x_wrap,
                    block_y_wrap,
                    block_z_wrap,
                    y_smear_adj,
                    block_y_adj
                ) / acc;
            }
            if (!(noise_result >= 1.0)) {
                min_noise_acc += generate_improved_noise(
                    noise_limit + 16, // Assumes min/max perlin noise has 16 levels
                    block_x_wrap,
                    block_y_wrap,
                    block_z_wrap,
                    y_smear_adj,
                    block_y_adj
                ) / acc;
            }
            acc /= 2.0;
        }
        double final_result;
        if (noise_result < 0.0) {
            final_result = min_noise_acc / (double)(1 << 16);
        } else if (noise_result > 1.0) {
            final_result = max_noise_acc / (double)(1 << 16);
        } else {
            final_result = lerp(noise_result, min_noise_acc / 512.0, max_noise_acc / 512.0) / 128.0;
        }

        //printf("old blend noise %lf %lf %lf %lf\n", noise_result, min_noise_acc, max_noise_acc, final_result);
        //fflush(stdout);
        regArray[finalStack->op_stack[pc].result_index] = final_result;
        goto *dispatch_table[finalStack->op_stack[--pc].op_type];
    }

    op_halt:
        // printf("%lf\n", regArray[0]);
        return regArray[0];

    return 0.0;
}