#include <stdbool.h>
#include ".\include\op_types.h"
#include ".\include\op_stack.h"
#include "stack_builder.h"
#include "test_cases.h"

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
    if (root->operationType == OP_CONST) return 0;
    int* finalDepths = (int*)malloc(sizeof(int) * root->numChildren);

    for (int i = 0; i < root->numChildren; i++) {
        if (root->allChildren[i]) finalDepths[i] = preorder_traversal_depth_count(root->allChildren[i]);
    }
    
    root->depth = max(finalDepths, root->numChildren);
    return root->depth + 1;
}

int generate_stack_recursive(EvalStack* evalStack, Node* node, int regPos) {
    if (node->operationType == OP_CONST) {
        // Allocate for new constant and add to const_args array
        // Note: realloc will eventually be removed because we will know the
        // number of constant args before this recursive traversal
        evalStack->const_args = realloc(evalStack->const_args, sizeof(double) * ++evalStack->const_args_len);
        evalStack->const_args[evalStack->const_args_len - 1] = node->value;
        return -evalStack->const_args_len;
    }
    // Keep running maximum register for later use
    if (regPos > evalStack->max_reg)
        evalStack->max_reg = regPos;

    // Allocate new stack frame
    StackFrame* currentFrame = (StackFrame*)malloc(sizeof(StackFrame));
    *currentFrame = (StackFrame){
        .op_type = node->operationType,
        .result_index = regPos
    };

    // Allocate for new frame and add to stack array
    // Note: realloc will also be eventually removed like above
    evalStack->op_stack = realloc(evalStack->op_stack, sizeof(StackFrame*) * ++evalStack->op_stack_len);
    evalStack->op_stack[evalStack->op_stack_len - 1] = currentFrame;
    // Execute type-based stack generation
    return node->generate_stack_recursive_from_op(evalStack, currentFrame, node, regPos);
}

EvalStack* generate_stack_wrapper(Node* node) {
    preorder_traversal_depth_count(node);

    EvalStack* evalStack = (EvalStack*)malloc(sizeof(EvalStack));
    // Initialize array pointers
    evalStack->op_stack = (StackFrame**)malloc(sizeof(StackFrame*));
    evalStack->op_stack_len = 0;
    evalStack->const_args = (double*)malloc(0);
    evalStack->const_args_len = 0;
    evalStack->max_reg = 0;

    generate_stack_recursive(evalStack, node, 0);

    return evalStack;
}

int one_arg_op_gen(EvalStack* evalStack, StackFrame* currentNode, Node* node, int regPos){
    currentNode->arg_indexes[0] = generate_stack_recursive(evalStack, node->allChildren[0], regPos);
    return regPos;
}

int two_args_op_gen(EvalStack* evalStack, StackFrame* currentNode, Node* node, int regPos) {
    if (node->allChildren[0] && node->allChildren[1]) {
        bool switchArgs = node->allChildren[0]->depth > node->allChildren[1]->depth;
        currentNode->arg_indexes[switchArgs] = generate_stack_recursive(evalStack, switchArgs ? node->allChildren[1] : node->allChildren[0], regPos + 1);
        currentNode->arg_indexes[!switchArgs] = generate_stack_recursive(evalStack, switchArgs ? node->allChildren[0] : node->allChildren[1], regPos);
    }
    return regPos;
}

int three_args_op_gen(EvalStack* evalStack, StackFrame* currentNode, Node* node, int regPos) {
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
            currentNode->arg_indexes[i] = generate_stack_recursive(evalStack, node->allChildren[i], regPos + (2 - i));
    }
    return regPos;
}

Node* create_leaf(int op_type, double theData) {
    Node* n    = (Node*)malloc(sizeof(Node));
    n->type    = NODE_LEAF;
    n->operationType = op_type;
    n->depth = 0;
    n->label   = 0;
    n->value = theData;
    return n;
}

Node* create_op(int op_type, Node** theChildren) {
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
        n->generate_stack_recursive_from_op = one_arg_op_gen;
        n->allChildren = (Node**)malloc(sizeof(Node*));
        n->allChildren[0] = theChildren[0];
        n->numChildren = 1;
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

    if (left_max_num_children > right_max_num_children)
        return left_max_num_children;
    else
        return right_max_num_children;
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

    while (rest != NULL) {
        // Prevent bad child dereferences
        if (rest->numChildren < 2) break;
        // FIX ONE ARG OPS CAUSING SEG FAULT

        int should_rotate = rotation_is_safe(rest);

        if (should_rotate) {
            Node* left_child = rest->allChildren[0];
            if (left_child && left_child->numChildren > rest->numChildren) {
                should_rotate = 0;
            }
        }
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
    int pos = s->op_stack[offset]->arg_indexes[arg_num];
    if (pos < 0)
        printf("%lf\n", s->const_args[abs(pos) - 1]);
    return pos < 0 ? s->const_args[abs(pos) - 1] : registers[pos];
}

double eval_stack(EvalStack* finalStack) {

    printf("Registers Used: %d\n", finalStack->max_reg+1);
    double* regArray = (double*)malloc(sizeof(double) * (finalStack->max_reg+1));

    for (int i = finalStack->op_stack_len - 1; i >= 0; i--)
    {
        double arg1;
        double arg2;
        switch (finalStack->op_stack[i]->op_type)
        {
        case OP_ADD:
            regArray[finalStack->op_stack[i]->result_index] = get_arg(finalStack, regArray, i, 0) + get_arg(finalStack, regArray, i, 1);
            break;
        case OP_MUL:
            regArray[finalStack->op_stack[i]->result_index] = get_arg(finalStack, regArray, i, 0) * get_arg(finalStack, regArray, i, 1);
            break;
        case OP_MIN:
            arg1 = get_arg(finalStack, regArray, i, 0);
            arg2 = get_arg(finalStack, regArray, i, 1);
            regArray[finalStack->op_stack[i]->result_index] = arg1 < arg2 ? arg1 : arg2;
            break;
        case OP_MAX:
            arg1 = get_arg(finalStack, regArray, i, 0);
            arg2 = get_arg(finalStack, regArray, i, 1);
            regArray[finalStack->op_stack[i]->result_index] = arg1 > arg2 ? arg1 : arg2;
            break;
        case OP_SHIFTED_NOISE:
            regArray[finalStack->op_stack[i]->result_index] = get_arg(finalStack, regArray, i, 0) + get_arg(finalStack, regArray, i, 1) + get_arg(finalStack, regArray, i, 2);
            break;
        case OP_ABS:
            regArray[finalStack->op_stack[i]->result_index] = abs(get_arg(finalStack, regArray, i, 0));
        default:
            break;
        }
        printf("%lf, %lf\n", regArray[0], regArray[1]);
    }
    printf("%lf\n", regArray[0]);
    return regArray[0];
}

int main() {
    register_op(OP_ADD, 3, 1);              // associative - ADD
    register_op(OP_MUL, 4, 1);              // associative - MULT
    register_op(OP_MIN, 1, 1);              // associative - MIN
    register_op(OP_MAX, 1, 1);              // non associative - MAX
    register_op(OP_SHIFTED_NOISE, 10, 0);   // non associative - SHIFTED_NOISE
    register_op(OP_ABS, 0, 0);              // non associative - ABS

    Node* root = tree_to_vine(min_and_max_test_case());
    EvalStack* theStack = generate_stack_wrapper(root);
    double resultOfOps = eval_stack(theStack);

    return 0;
}