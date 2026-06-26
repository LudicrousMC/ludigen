#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include "op_types.h"
#include "op_stack.h"
#include "stack_builder.h"
#include "test_cases.h"
#include "util.h"

#define MAX_OPS 30
OpInfo op_table[MAX_OPS];
int    op_count;

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

int preorder_traversal_depth_count(Node* root) {
    if (root->numChildren == 0) return 0;
    int finalDepths[root->numChildren];

    for (int i = 0; i < root->numChildren; i++) {
        if (root->allChildren[i]) finalDepths[i] = preorder_traversal_depth_count(root->allChildren[i]);
    }

    root->depth = max(finalDepths, root->numChildren);
    return root->depth + 1;
}

// Finds allocation requirements of op_stack and const_args for EvalStack
EvalStackAllocReqs preorder_traversal_alloc_reqs(Node* root) {
    // Return requirements for leaf nodes that don't have children
    switch (root->operationType)
    {
    case OP_CONST:
        return (EvalStackAllocReqs){0,1};
    case OP_Y_CLAMPED_GRAD:
        return (EvalStackAllocReqs){1,4};
    default:
        break;
    }

    // If this node is an actual operation, reserve 1 length in op_stack
    EvalStackAllocReqs reqs = {1,0};
    // Account for ops that have extra constants
    switch (root->operationType)
    {
    case OP_SPLINE:
        if (root->values[0] == OP_SUBTYPE_SPLINE_POINT) {
            if (root->allChildren[0]->operationType == OP_CONST)
                return (EvalStackAllocReqs){0,3}; // Omit spline point op if it holds a constant value
            reqs.const_args_len += 3; // derivative, location, and stack index constants
        } else if (root->values[0] == OP_SUBTYPE_SPLINE)
            reqs.const_args_len += 2; // number of points and stack return index constant
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

int generate_stack_recursive(EvalStack* evalStack, Node* node, int regPos) {
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
    evalStack->op_stack = (StackFrame*)malloc(sizeof(StackFrame) * evalStackAlloc.op_stack_len);
    evalStack->op_stack_len = 0;
    evalStack->const_args = (double*)malloc(sizeof(double) * evalStackAlloc.const_args_len);
    evalStack->const_args_len = 0;
    evalStack->max_reg = 0;

    generate_stack_recursive(evalStack, node, 0);

    return evalStack;
}

int spline_op_gen(EvalStack* evalStack, size_t currentFrameIndex, Node* node, int regPos) {
    // Assume current frame is for first spline point
    int num_points = node->values[1];
    // Constants Length is (number of points * (location, deriv, stack index)) + num_points constant
    int num_constants = (num_points * 3) + 1;
    double constants[num_constants];
    constants[0] = num_points;
    int pointFrameIndexes[num_points];

    for (int i = 0, j = 1; i < num_points; i++, j++) {
        Node* point = node->allChildren[i];
        constants[j] = point->values[1]; // Add point location
        constants[j + num_points] = point->values[2]; // Add point derivative
        size_t pointFrameIndex = evalStack->op_stack_len - 1; // Save reference to current point StackFrame
        int index = generate_stack_recursive(evalStack, point->allChildren[0], regPos + 3); // Generate point value stack

        // If point value is constant, store index as negative const_args index, else store op_stack index for evaluating point
        if (index < 0) {
            pointFrameIndexes[i] = -1;
            // Undo spline add
            evalStack->op_stack_len--;
            constants[j + (num_points*2)] = (double)index;
        } else {
            pointFrameIndexes[i] = pointFrameIndex;
            constants[j + (num_points*2)] = evalStack->op_stack_len;
            // Save value reg position in spline point args
            evalStack->op_stack[pointFrameIndex].arg_indexes[0] = index;
            // Store return frame index as negative const_index
            evalStack->op_stack[pointFrameIndex].const_index = -(currentFrameIndex + 1);
        }

        evalStack->op_stack[evalStack->op_stack_len++] = (StackFrame){ .op_type = point->operationType, .result_index = regPos };
    }

    for (int i = 0; i < num_points; i++) {
        int index = pointFrameIndexes[i];
        if (index != -1) {
            // Store main spline frame in second arg index of points
            evalStack->op_stack[index].arg_indexes[1] = evalStack->op_stack_len - 1;
        }
    }

    // Add constants to const_args stack
    evalStack->op_stack[evalStack->op_stack_len - 1].const_index = evalStack->const_args_len;
    for (int i = 0; i < num_constants; i++) {
        evalStack->const_args[evalStack->const_args_len++] = constants[i];
    }
    // Evaluate coordinate first on stack
    if (node->allChildren[0]) {
        evalStack->op_stack[evalStack->op_stack_len - 1].arg_indexes[0] = generate_stack_recursive(evalStack, node->allChildren[num_points], regPos);
    }

    // Store return frame index as second arg
    evalStack->op_stack[evalStack->op_stack_len - 1].arg_indexes[1] = currentFrameIndex;
    return regPos;
}

int y_clamp_op_gen(EvalStack* evalStack, size_t currentFrameIndex, Node* node, int regPos) {
    evalStack->op_stack[currentFrameIndex].const_index = evalStack->const_args_len;
    for (int i = 0; i < 4; i++)
        evalStack->const_args[evalStack->const_args_len++] = node->values[i];
    return regPos;
}

int one_arg_op_gen(EvalStack* evalStack, size_t currentFrameIndex, Node* node, int regPos) {
    if (node->allChildren[0]) {
        evalStack->op_stack[currentFrameIndex].arg_indexes[0] = generate_stack_recursive(evalStack, node->allChildren[0], regPos);
    }
    return regPos;
}

int two_args_op_gen(EvalStack* evalStack, size_t currentFrameIndex, Node* node, int regPos) {
    if (node->allChildren[0] && node->allChildren[1]) {
        bool switchArgs = node->allChildren[0]->depth > node->allChildren[1]->depth;
        evalStack->op_stack[currentFrameIndex].arg_indexes[switchArgs] = generate_stack_recursive(evalStack, switchArgs ? node->allChildren[1] : node->allChildren[0], regPos + 1);
        evalStack->op_stack[currentFrameIndex].arg_indexes[!switchArgs] = generate_stack_recursive(evalStack, switchArgs ? node->allChildren[0] : node->allChildren[1], regPos);
    }
    return regPos;
}

int three_args_op_gen(EvalStack* evalStack, size_t currentFrameIndex, Node* node, int regPos) {
    if (node->allChildren[0] && node->allChildren[1] && node->allChildren[2]) {
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
    }
    return regPos;
}

Node* create_leaf(int op_type, double* theData) {
    Node* n    = (Node*)malloc(sizeof(Node));
    n->type    = NODE_LEAF;
    n->operationType = op_type;
    n->depth = 0;
    n->label   = 0;
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
    n->numChildren = 0;
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
    case OP_SHIFTED_NOISE:
        n->generate_stack_recursive_from_op = three_args_op_gen;
        n->allChildren = (Node**)malloc(3 * sizeof(Node*));
        n->allChildren[0] = theChildren[0];
        n->allChildren[1] = theChildren[1];
        n->allChildren[2] = theChildren[2];
        n->numChildren = 3;
        break;
    case OP_ABS:
    case OP_SQUARE:
    case OP_CUBE:
    case OP_HALF_NEGATIVE:
    case OP_QUARTER_NEGATIVE:
    case OP_INVERT:
    case OP_SQUEEZE:
    case OP_CLAMP:
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
    if (root->numChildren != 2 || !lookup_op(root->operationType)->associative)
        return root->numChildren;

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
    larger_arg_right(root);
    if (root->operationType == OP_CONST) return root;
    Node pseudo_root;
    pseudo_root.allChildren     = (Node*[2]){0};
    pseudo_root.allChildren[1]  = root;
    pseudo_root.allChildren[0]  = NULL;
    pseudo_root.type            = NODE_OP;
    pseudo_root.operationType   = 0;
    pseudo_root.label           = 0;

    Node* tail = &pseudo_root;
    Node* rest = tail->allChildren[1];
    if (root->numChildren > 2 || root->operationType == OP_SPLINE) {
        // Recurse for nodes with more than two children
        for (int i = 0; i < root->numChildren; i++) {
            root->allChildren[i] = tree_to_vine(root->allChildren[i]);
        }
        return root;
    } else if (root->numChildren == 1) {
        // Passthrough for nodes with one child
        root->allChildren[0] = tree_to_vine(root->allChildren[0]);
        return root;
    }

    while (rest != NULL) {
        if (rest->numChildren < 2) break; // Prevent bad child dereferences

        //int should_rotate = rotation_is_safe(rest);

        /*if (should_rotate) {
            Node* left_child = rest->allChildren[0];
            if (left_child && left_child->numChildren > rest->numChildren) {
                should_rotate = 0;
            }
        }*/
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

double evaluate_stack(EvalStack* finalStack, double x, double y, double z) {

    // printf("Registers Used: %d\n", finalStack->max_reg+1);
    double regArray[finalStack->max_reg+1];

    for (int i = finalStack->op_stack_len - 1; i >= 0; i--)
    {
        double arg1;
        double arg2;
        switch (finalStack->op_stack[i].op_type)
        {
        case OP_ADD:
            regArray[finalStack->op_stack[i].result_index] = get_arg(finalStack, regArray, i, 0) + get_arg(finalStack, regArray, i, 1);
            break;
        case OP_MUL:
            regArray[finalStack->op_stack[i].result_index] = get_arg(finalStack, regArray, i, 0) * get_arg(finalStack, regArray, i, 1);
            break;
        case OP_MIN:
            arg1 = get_arg(finalStack, regArray, i, 0);
            arg2 = get_arg(finalStack, regArray, i, 1);
            regArray[finalStack->op_stack[i].result_index] = arg1 < arg2 ? arg1 : arg2;
            break;
        case OP_MAX:
            arg1 = get_arg(finalStack, regArray, i, 0);
            arg2 = get_arg(finalStack, regArray, i, 1);
            regArray[finalStack->op_stack[i].result_index] = arg1 > arg2 ? arg1 : arg2;
            break;
        case OP_ABS:
            regArray[finalStack->op_stack[i].result_index] = fabs(get_arg(finalStack, regArray, i, 0));
            break;
        case OP_SQUARE:
            arg1 = get_arg(finalStack, regArray, i, 0);
            regArray[finalStack->op_stack[i].result_index] = arg1 * arg1;
            break;
        case OP_CUBE:
            arg1 = get_arg(finalStack, regArray, i, 0);
            regArray[finalStack->op_stack[i].result_index] = arg1 * arg1 * arg1;
            break;
        case OP_HALF_NEGATIVE:
            arg1 = get_arg(finalStack, regArray, i, 0);
            regArray[finalStack->op_stack[i].result_index] = arg1 < 0.0 ? arg1 * 0.5 : arg1;
            break;
        case OP_INVERT:
            regArray[finalStack->op_stack[i].result_index] = 1.0 / get_arg(finalStack, regArray, i, 0);
            break;
        case OP_QUARTER_NEGATIVE:
            arg1 = get_arg(finalStack, regArray, i, 0);
            regArray[finalStack->op_stack[i].result_index] = arg1 < 0.0 ? arg1 * 0.25 : arg1;
            break;
        case OP_SQUEEZE:
            double clamped_val = fmin(fmax(get_arg(finalStack, regArray, i, 0), -1.0), 1.0);
            regArray[finalStack->op_stack[i].result_index] = (clamped_val / 2.0) - ((clamped_val * clamped_val * clamped_val) / 24.0);
            break;
        case OP_CLAMP:
            regArray[finalStack->op_stack[i].result_index] = fmin(fmax(get_arg(finalStack, regArray, i, 0), -1.0), 1.0);
            break;
        case OP_Y_CLAMPED_GRAD:
            double fromY = get_const(finalStack, i, 0);
            double toY = get_const(finalStack, i, 1);
            double fromValue = get_const(finalStack, i, 2);
            double toValue = get_const(finalStack, i, 3);
            regArray[finalStack->op_stack[i].result_index] = clamped_map(y, fromY, toY, fromValue, toValue);
            break;
        case OP_SHIFTED_NOISE:
            regArray[finalStack->op_stack[i].result_index] = get_arg(finalStack, regArray, i, 0) + get_arg(finalStack, regArray, i, 1) + get_arg(finalStack, regArray, i, 2);
            break;
        case OP_SPLINE:
            // If const_index is non negative, treat as main spline, else treat as spline point
            if (finalStack->op_stack[i].const_index >= 0) {
                double coord = get_arg(finalStack, regArray, i, 0);
                int num_points = get_const(finalStack, i, 0);
                int len = num_points;
                // Search for point
                int point_index = 0;
                while (len > 0) {
                    int half = len / 2;
                    int mid = point_index + half;
                    if (coord < get_const(finalStack, i, mid + 1)) {
                        len = half;
                    } else {
                        point_index = mid + 1;
                        len -= half + 1;
                    }
                }
                point_index -= 1;

                if (point_index < 0) {
                    // If first point (single point)
                    int first_point_index = get_const(finalStack, i, (num_points * 2) + 1);
                    // If first point is constant (negative), save result in register and go to return frame,
                    // else go to point value start frame
                    if (first_point_index < 0) {
                        double first_point_loc = get_const(finalStack, i, 1);
                        double first_point_deriv = get_const(finalStack, i, num_points + 1);
                        double first_point_value = finalStack->const_args[abs(first_point_index) - 1];

                        regArray[finalStack->op_stack[i].result_index] = linear_ext_if_non_zero(first_point_deriv, first_point_loc, coord, first_point_value);
                        // Set stack pos to return spline frame
                        i = finalStack->op_stack[i].arg_indexes[1];
                    } else {
                        // Set spline state in registers
                        regArray[finalStack->op_stack[i].result_index] = coord;
                        regArray[finalStack->op_stack[i].result_index + 1] = 0.0; // Single point indicator
                        regArray[finalStack->op_stack[i].result_index + 2] = 0.0; // point index
                        i = first_point_index;
                    }
                } else if (point_index == num_points - 1) {
                    // If last point (single point)
                    int last_point_index = get_const(finalStack, i, (num_points * 3));
                    // If last point is constant (negative), save result in register and go to return frame,
                    // else go to point value start frame
                    if (last_point_index < 0) {
                        double last_point_loc = get_const(finalStack, i, num_points);
                        double last_point_deriv = get_const(finalStack, i, (num_points * 2));
                        double last_point_value = finalStack->const_args[abs(last_point_index) - 1];

                        regArray[finalStack->op_stack[i].result_index] = linear_ext_if_non_zero(last_point_deriv, last_point_loc, coord, last_point_value);
                        // Set stack pos to return spline frame
                        i = finalStack->op_stack[i].arg_indexes[1];
                    } else {
                        // Set spline state in registers
                        regArray[finalStack->op_stack[i].result_index] = coord;
                        regArray[finalStack->op_stack[i].result_index + 1] = 0.0; // Single point indicator
                        regArray[finalStack->op_stack[i].result_index + 2] = (double)(num_points - 1); // point index
                        i = last_point_index;
                    }
                } else {
                    // If between points, interpolate (double point)
                    int point1_index = get_const(finalStack, i, (num_points * 2) + 1 + point_index)
                    int point2_index = get_const(finalStack, i, (num_points * 2) + 2 + point_index);
                    // If both points are constants (negative), compute spline and go to return frame,
                    // else if next point is constant (negative), save result in register and start on first point
                    // else go to point value start frame
                    if (point2_index < 0 && point1_index < 0) {
                        double point1_loc = get_const(finalStack, i, 1 + point_index);
                        double point1_deriv = get_const(finalStack, i, num_points + 1 + point_index);
                        double point1_val = finalStack->const_args[abs(point1_index) - 1];
                        double point2_loc = get_const(finalStack, i, 2 + point_index);
                        double point2_deriv = get_const(finalStack, i, num_points + 2 + point_index);
                        double point2_val = finalStack->const_args[abs(point2_index) - 1];

                        regArray[finalStack->op_stack[i].result_index] = spline_compute(point1_loc, point1_deriv, point1_val, point2_loc, point2_deriv, point2_val, coord);
                        i = finalStack->op_stack[i].arg_indexes[1];
                    } else {

                    }
                }
            } else {
                // Spline point case
                int main_spline_reg_index = finalStack->op_stack[i].result_index;
                int main_spline_stack_index = finalStack->op_stack[i].arg_indexes[1];
                // If second reg position is 0.0 (single point indicator)
                if (regArray[main_spline_reg_index + 1] == 0.0) {
                    int point_index = regArray[main_spline_reg_index + 2];
                    int num_points = get_const(finalStack, main_spline_stack_index, 0);
                    double coord = regArray[finalStack->op_stack[i].result_index];
                    double point_loc = get_const(finalStack, main_spline_stack_index, point_index + 1);
                    double point_deriv = get_const(finalStack, main_spline_stack_index, num_points + point_index + 1);
                    double point_value = get_arg(finalStack, regArray, i, 0);
                    regArray[finalStack->op_stack[i].result_index] = linear_ext_if_non_zero(point_deriv, point_loc, coord, point_value);
                    i = -(finalStack->op_stack[i].const_index + 1);
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
        default:
            break;
        }
        printf("%lf, %lf %lf %lf\n", regArray[0], regArray[1], regArray[2], regArray[3]);
    }
    // printf("%lf\n", regArray[0]);
    return regArray[0];
}

int main() {
    register_op(OP_ADD, 2, 1);                  // associative
    register_op(OP_MUL, 3, 1);                  // associative
    register_op(OP_MIN, 1, 1);                  // associative
    register_op(OP_MAX, 1, 1);                  // associative
    register_op(OP_ABS, 0, 0);                  // non associative
    register_op(OP_SQUARE, 0, 0);               // non associative
    register_op(OP_CUBE, 0,0);                  // non associative
    register_op(OP_HALF_NEGATIVE, 0, 0);        // non associative
    register_op(OP_QUARTER_NEGATIVE, 0, 0);     // non associative
    register_op(OP_INVERT, 0, 0);               // non associative
    register_op(OP_SQUEEZE, 0, 0);              // non associative
    register_op(OP_CLAMP, 0, 0);                // non associative
    register_op(OP_Y_CLAMPED_GRAD, 0, 0);       // non associative
    register_op(OP_SHIFTED_NOISE, 10, 0);       // non associative - FIX THIS EVALUATION, YOU DAMNNED TWAT
    register_op(OP_SPLINE, 0, 0);               // non associative
    register_op(OP_BLEND_DENSITY, 0, 0);        // non associative
    register_op(OP_BLEND_OFFSET, 0, 0);         // non associative
    register_op(OP_BLEND_ALPHA, 0, 0);          // non associative
    register_op(OP_CACHE_ONCE, 0, 0);           // non associative
    register_op(OP_FLAT_CACHE, 0, 0);           // non associative
    register_op(OP_CACHE_2D, 0, 0);             // non associative
    register_op(OP_CACHE_ALL_IN_CELL, 0, 0);    // non associative
    register_op(OP_END_ISLANDS, 0, 0);          // non associative

    // Node* root = tree_to_vine(associativity_test_case());
    // EvalStack* theStack = generate_stack_wrapper(root);
    // double resultOfOps = evaluate_stack(theStack);
    run_all_test_cases();

    return 0;
}