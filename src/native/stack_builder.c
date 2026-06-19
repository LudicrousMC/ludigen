#include "../headers/stack_builder.h"

int max(int *arr, int n) {
    int m = arr[0];
    for (int i = 1; i < n; i++)
        if (arr[i] > m) m = arr[i];
    return m;
}

// Register an operator with a priority and associativity flag.
void register_op(DensityFnType op, int priority, int associative) {
    for (int i = 0; i < op_count; i++) {
        if (op_table[i].op == op) {          // update existing entry
            op_table[i].priority    = priority;
            op_table[i].associative = associative;
            return;
        }
    }
    if (op_count < MAX_OPS) {
        op_table[op_count++] = (OpInfo){ op, priority, associative };
    }
}

// Returns NULL if the operator is not registered.
OpInfo* lookup_op(DensityFnType op) {
    for (int i = 0; i < op_count; i++) {
        if (op_table[i].op == op) return &op_table[i];
    }
    return NULL;
}

int preorder_traversal_depth_count(Node* root) {
    int finalDepthLeft = 0;
    int finalDepthRight = 0;
    if (root->left) finalDepthLeft = preorder_traversal_depth_count(root->left);
    if (root->right) finalDepthRight = preorder_traversal_depth_count(root->right);
    if (finalDepthLeft > finalDepthRight)
        root->depth = finalDepthLeft;
    else
        root->depth = finalDepthRight;
    return root->depth + 1;
}

void push_to_stack(LinkedStackFrame** stack, LinkedStackFrame* input) {
    input->nextFrame = *stack;
    *stack = input;
}

int generate_stack_recursive(EvalStack* evalStack, Node* node, int regPos) {
    LinkedStackFrame* currentNode = (LinkedStackFrame*)malloc(sizeof(LinkedStackFrame));
    currentNode->op_type = node->operationType;
    if (node->left && node->right) currentNode->reg_index = regPos;
    else currentNode->reg_index = 0;

    if (node->operationType != -1)
        push_to_stack(evalStack->stack, currentNode);
    else {
        evalStack->const_args[evalStack->const_args_len - 1] = node->value;
        evalStack->const_args = realloc(evalStack->const_args, sizeof(double) * ++evalStack->const_args_len);
        return -(evalStack->const_args_len - 1);
    }

    return node->generate_stack_recursive_from_op(evalStack, currentNode, node, regPos);
}

EvalStack* generate_stack_wrapper(Node* node) {
    preorder_traversal_depth_count(node);

    EvalStack* evalStack = (EvalStack*)malloc(sizeof(EvalStack));
    evalStack->stack = (LinkedStackFrame**)malloc(sizeof(LinkedStackFrame*));
    *evalStack->stack = (LinkedStackFrame*)malloc(sizeof(LinkedStackFrame));
    (*evalStack->stack)->nextFrame = NULL;
    evalStack->const_args_len = 1;
    evalStack->const_args = (double*)malloc(sizeof(double));

    generate_stack_recursive(evalStack, node, 0);

    return evalStack;
}

int two_args_op_gen(EvalStack* evalStack, LinkedStackFrame* currentNode, Node* node, int regPos) {
    if (node->left && node->right) {
        if (node->left->depth > node->right->depth) {
            currentNode->arg_indexes[1] = generate_stack_recursive(evalStack, node->right, regPos + 1);
            currentNode->arg_indexes[0] = generate_stack_recursive(evalStack, node->left, regPos);
        }
        else {
            currentNode->arg_indexes[0] = generate_stack_recursive(evalStack, node->left, regPos + 1);
            currentNode->arg_indexes[1] = generate_stack_recursive(evalStack, node->right, regPos);
        }
    }
    return regPos;
}

Node* create_leaf(DensityFnType val, double theData) {
    Node* n    = (Node*)malloc(sizeof(Node));
    n->type    = NODE_LEAF;
    n->operationType = val;
    n->depth = 0;
    n->label   = 0;
    n->value = theData;
    n->left    = n->right = NULL;
    return n;
}

Node* create_op(DensityFnType op, Node* left, Node* right) {
    Node* n    = (Node*)malloc(sizeof(Node));
    n->type    = NODE_OP;
    n->operationType = op;
    n->depth = 0;
    n->label   = 0;
    n->left    = left;
    n->right   = right;

    switch (op)
    {
    case 0:
    case 1:
        n->generate_stack_recursive_from_op = two_args_op_gen;
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
    Node* left_child = rest->left;
    if (!left_child || left_child->type == NODE_LEAF) return 0;

    OpInfo* op_rest  = lookup_op(rest->operationType);
    OpInfo* op_left  = lookup_op(left_child->operationType);
    if (!op_rest || !op_left) return 0;     // unknown operators → be conservative

    // The left child must bind no more tightly than its parent.
    // If it binds MORE tightly, hoisting it would change precedence.
    if (op_left->priority > op_rest->priority) return 0;

    // If same priority, the operator must be associative.
    if (op_left->priority == op_rest->priority && !op_left->associative) return 0;

    return 1;
}

void rotate_right(Node* tail) {
    Node* old_root   = tail->right;
    Node* new_root   = old_root->left;
    old_root->left   = new_root->right;
    new_root->right  = old_root;
    tail->right      = new_root;
}

Node* tree_to_vine(Node* root) {
    Node pseudo_root;
    pseudo_root.right    = root;
    pseudo_root.left     = NULL;
    pseudo_root.type     = NODE_OP;
    pseudo_root.operationType = 0;
    pseudo_root.label    = 0;

    Node* tail = &pseudo_root;
    Node* rest = tail->right;

    while (rest != NULL) {
        if (rotation_is_safe(rest)) {
            rotate_right(tail);
            rest = tail->right;   // re-examine the newly promoted node
        } else {
            // Cannot rotate here — skip down the right spine
            tail = rest;
            rest = rest->right;
        }
    }

    return pseudo_root.right;
}

Node* larger_opt_test_case() {
    Node* add1 = create_op(0, create_leaf(-1, 8), create_leaf(-1, 9));
    Node* add2 = create_op(0, create_leaf(-1, 10), create_leaf(-1, 11));
    Node* add3 = create_op(0, create_leaf(-1, 6), create_leaf(-1, 7));
    Node* add4 = create_op(0, add2, add3);
    Node* add5 = create_op(0, create_leaf(-1, 12), create_leaf(-1, 13));
    Node* add6 = create_op(0, add4, add5);
    Node* root = create_op(0, add1, add6);
    return root;
}

Node* non_associativity_test_case() {
    Node* min1 = create_op(2, create_leaf(-1, 8), create_leaf(-1, 9));
    Node* min2 = create_op(2, create_leaf(-1, 10), create_leaf(-1, 11));
    Node* root = create_op(2, min1, min2);
    return root;
}

GenerateStackReturn generate_stack(Node* root) {
    EvalStack* theStack = generate_stack_wrapper(root);
    LinkedStackFrame* theTRUEstack = *theStack->stack;
    StackFrame* stackArray = (StackFrame*)malloc(sizeof(StackFrame));
    int theIndex;
    int neededRegs = 0;
    for (int i = 0; theTRUEstack->nextFrame; i++)
    {
        if (neededRegs < theTRUEstack->reg_index)
            neededRegs = theTRUEstack->reg_index;
        stackArray[i].reg_index = theTRUEstack->reg_index;
        stackArray[i].op_type = theTRUEstack->op_type;
        memcpy(stackArray[i].arg_indexes, theTRUEstack->arg_indexes, sizeof(int[3]));
        printf("%d", theTRUEstack->reg_index);
        stackArray = realloc(stackArray, (sizeof(StackFrame) * i) + (sizeof(StackFrame) * 2));
        theTRUEstack = theTRUEstack->nextFrame;
        theIndex = i;
    }
    neededRegs++;

    return (GenerateStackReturn) {neededRegs, theIndex, stackArray, theStack};
}


int main() {

    register_op(0, 1, 1);   // lowest priority, associative - ADD
    register_op(1, 2, 1);   // higher priority, associative - MULT
    register_op(2, 1, 0);   // lowest priority, non-associative - MIN

    Node* root = tree_to_vine(larger_opt_test_case());

    GenerateStackReturn theFullStack = generate_stack(root);

    double* regArray = (double*)malloc(sizeof(double) * theFullStack.returnedNeededRegs);

    for (int i = 0; i <= theFullStack.returnedIndex; i++)
    {
        switch (theFullStack.returnedStackFrames[i].op_type)
        {
        case 0:
            double arg1, arg2;
            if (theFullStack.returnedStackFrames[i].arg_indexes[0] < 0) {
                arg1 = theFullStack.returnedEvalStack->const_args[abs(theFullStack.returnedStackFrames[i].arg_indexes[0]) - 1];
            }
            else
                arg1 = regArray[theFullStack.returnedStackFrames[i].arg_indexes[0]];
            if (theFullStack.returnedStackFrames[i].arg_indexes[1] < 0) {
                arg2 = theFullStack.returnedEvalStack->const_args[abs(theFullStack.returnedStackFrames[i].arg_indexes[1]) - 1];
            }
            else
                arg2 = regArray[theFullStack.returnedStackFrames[i].arg_indexes[1]];
            
            printf("\n%d", theFullStack.returnedStackFrames[i].reg_index);
            regArray[theFullStack.returnedStackFrames[i].reg_index] = arg1 + arg2;
            break;
        default:
            break;
        }
    }
    printf("\n%lf", regArray[0]);

    return 0;
}