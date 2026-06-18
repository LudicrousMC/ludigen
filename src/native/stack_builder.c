#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../headers/func_state.h"


int max(int *arr, int n) {
    int m = arr[0];
    for (int i = 1; i < n; i++)
        if (arr[i] > m) m = arr[i];
    return m;
}

// ─────────────────────────────────────────────
//           OPERATOR PRIORITY TABLE
// ─────────────────────────────────────────────

typedef struct {
    DensityFnType   op;
    int             priority;      // higher number = tighter binding
    int             associative;   // 1 = safe to rotate, 0 = do not rotate
} OpInfo;

#define MAX_OPS 16
OpInfo op_table[MAX_OPS];
int    op_count = 0;

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

// ─────────────────────────────────────────────
//                   AST
// ─────────────────────────────────────────────

typedef struct LinkedStackFrame {
    struct LinkedStackFrame* nextFrame;
    int reg_index;
    int op_type;
    int arg_indexes[3];
} LinkedStackFrame;

typedef struct StackFrame {
    int reg_index;
    int op_type;
    int arg_indexes[3];
} StackFrame;

typedef struct EvalStack {
    LinkedStackFrame** stack;
    double* const_args;
    int const_args_len;
} EvalStack;

typedef enum { NODE_LEAF, NODE_OP } NodeType;

typedef struct Node {
    NodeType        type;
    DensityFnType   operationType;
    int             label;
    int             depth;
    double          value;
    struct Node*    left;
    struct Node*    right;
} Node;

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
    return n;
}

// ─────────────────────────────────────────────
//                  TRAVERSAL
// ─────────────────────────────────────────────

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

// ─────────────────────────────────────────────
//           STACK GENERATION + HELPERS
// ─────────────────────────────────────────────

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

// ─────────────────────────────────────────────
//                REGISTER POOL
// ─────────────────────────────────────────────

int reg_stack[100];
int top     = -1;
int min_top = 100;

void push_register(int r)  { reg_stack[++top] = r; }
int  pop_register()        {
    int r = reg_stack[top--];
    if (top < min_top) min_top = top;
    return r;
}

// ─────────────────────────────────────────────
//        PHASE 1 – SETHI-ULLMAN LABELING
// ─────────────────────────────────────────────

void compute_labels(Node* root) {
    if (!root) return;
    compute_labels(root->left);
    compute_labels(root->right);

    if (root->type == NODE_LEAF) {
        root->label = 1;
    } else {
        int l = root->left  ? root->left->label  : 0;
        int r = root->right ? root->right->label : 0;
        root->label = (l == r) ? l + 1 : (l > r ? l : r);
    }
}

// ─────────────────────────────────────────────
//        PHASE 2 – CODE GENERATION
// ─────────────────────────────────────────────

const char* op_mnemonic(char op) {
    switch (op) {
        case 0: return "ADD";
        case 1: return "MUL";
        case 2: return "MIN";
        default:  return "OP?";
    }
}

void generate_code(Node* root) {
    if (!root) return;

    if (root->type == NODE_LEAF) {
        int r = pop_register();
        printf("LOAD R%d, %c\n", r, root->operationType);
        push_register(r);
        return;
    }

    int left_lbl  = root->left  ? root->left->label  : 0;
    int right_lbl = root->right ? root->right->label : 0;

    // ── CASE 1: both children are leaves ────────────────────────────────────
    // Load left into a register, apply right directly from memory.
    if (root->left->type == NODE_LEAF && root->right->type == NODE_LEAF) {
        int r = pop_register();
        printf("LOAD R%d, %c\n", r, root->left->operationType);
        printf("%s R%d, R%d, %c\n",
               op_mnemonic(root->operationType), r, r, root->right->operationType);
        push_register(r);
    }
    // ── CASE 2 (NEW): left is a leaf, right is an op (vine chain) ───────────
    // The vine pattern: leaf OP (leaf OP (leaf OP ...)). 
    // Evaluate the right subtree first into a register, then fold the left
    // leaf into that same register — no second register ever needed.
    else if (root->left->type == NODE_LEAF && root->right->type == NODE_OP) {
        generate_code(root->right);          // right subtree result sits on top of stack
        int r = pop_register();              // grab that result register
        printf("%s R%d, R%d, %c\n",
               op_mnemonic(root->operationType), r, r, root->left->operationType);
        push_register(r);                   // result back in the same register
    }
    // ── CASE 3: left subtree is heavier or equal ────────────────────────────
    else if (left_lbl >= right_lbl) {
        generate_code(root->left);
        int r_left = pop_register();
        generate_code(root->right);
        int r_right = pop_register();
        printf("%s R%d, R%d, R%d\n",
               op_mnemonic(root->operationType), r_left, r_left, r_right);
        push_register(r_right);
        push_register(r_left);
    }
    // ── CASE 4: right subtree is heavier ────────────────────────────────────
    else {
        generate_code(root->right);
        int r_right = pop_register();
        generate_code(root->left);
        int r_left = pop_register();
        printf("%s R%d, R%d, R%d\n",
               op_mnemonic(root->operationType), r_left, r_left, r_right);
        push_register(r_right);
        push_register(r_left);
    }
}

// ─────────────────────────────────────────────
//           REGISTER USAGE REPORT
// ─────────────────────────────────────────────

void report_registers_used(int pool_size) {
    int peak = pool_size - (min_top + 1);
    printf("\n--- Register Usage Report ---\n");
    printf("Register pool size : %d\n", pool_size);
    printf("Peak registers used: %d\n", peak);
    printf("Registers spared   : %d\n", pool_size - peak);
}

// ─────────────────────────────────────────────
//     TREE-TO-VINE  (semantics-preserving)
// ─────────────────────────────────────────────

/*
 * A right-rotation at `tail` is safe only when:
 *
 *   1. The child node (tail->right, called `rest`) has a left subtree.
 *   2. Both `rest` and its left child are OP nodes.
 *   3. The left child's operator has LOWER OR EQUAL priority than `rest`'s
 *      operator — meaning the left child was meant to bind less tightly, so
 *      hoisting it above `rest` does not change evaluation order.
 *   4. The left child's operator is flagged as associative in the op_table,
 *      because we are effectively rewriting  (A op1 B) op2 C
 *      into  A op1 (B op2 C)  — valid only when op1 == op2 and associative,
 *      OR when op1 binds strictly looser than op2 (priority check covers this).
 *
 * If any condition fails we advance `tail` without rotating, leaving that
 * part of the tree untouched.
 */
static int rotation_is_safe(Node* rest) {
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

static void rotate_right(Node* tail) {
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

// ─────────────────────────────────────────────
//              PRETTY PRINTER
// ─────────────────────────────────────────────

void print_tree(Node* root, int space) {
    if (!root) return;
    space += 5;
    print_tree(root->right, space);
    printf("\n");
    for (int i = 5; i < space; i++) printf(" ");
    printf("%c(L:%d)\n", root->operationType, root->label);
    print_tree(root->left, space);
}

// ─────────────────────────────────────────────
//                   MAIN
// ─────────────────────────────────────────────

int main() {

    // ── 1. Register operators with user-defined priorities ──────────────────
    //
    //   register_op(operator, priority, is_associative)
    //
    //   priority    : higher number = tighter binding (evaluated first)
    //   associative : 1 = reordering is safe, 0 = preserve original grouping
    //
    register_op(0, 1, 1);   // lowest priority, associative - ADD
    register_op(1, 2, 1);   // higher priority, associative - MULT
    register_op(2, 1, 0);   // lowest priority, non-associative - MIN

    // ── 2. Build register pool ──────────────────────────────────────────────
    int pool_size = 4;
    for (int i = pool_size - 1; i >= 0; i--) push_register(i);
    min_top = top;

    // ── 3. TESTING FOR REBALANCING (should take 2 regs): ((a + b) * (c + d)) + ((e + f) * g) ─────────────────
    //  Left half:  (a + b) * (c + d)
    // Node* ab   = create_op(0, create_leaf('a'), create_leaf('b'));  // label 2
    // Node* cd   = create_op(0, create_leaf('c'), create_leaf('d'));  // label 2
    // Node* left = create_op(1, ab, cd);                             // label 3

    // //  Right half: (e + f) * g
    // Node* ef    = create_op(0, create_leaf('e'), create_leaf('f')); // label 2
    // Node* right = create_op(1, ef, create_leaf('g'));              // label 2

    // //  Root: left + right
    // Node* root  = create_op(0, left, right);                       // label 4 → 3 after vine

    // TEST CASE FOR LARGER FIRST OPT (should take 2 regs, despite only needing 1 in the best case):
    Node* add1 = create_op(0, create_leaf(-1, 8), create_leaf(-1, 9));
    Node* add2 = create_op(0, create_leaf(-1, 10), create_leaf(-1, 11));
    Node* add3 = create_op(0, create_leaf(-1, 6), create_leaf(-1, 7));
    Node* add4 = create_op(0, add2, add3);
    Node* add5 = create_op(0, create_leaf(-1, 12), create_leaf(-1, 13));
    Node* add6 = create_op(0, add4, add5);
    Node* root = create_op(0, add1, add6);
    root = tree_to_vine(root);

    // TEST CASE FOR NON ASSOCIATIVITY (should be non-optimizable, takes 2 registers) ((a - b) - (c - d)):
    // Node* min1 = create_op(2, create_leaf(8), create_leaf(9));
    // Node* min2 = create_op(2, create_leaf(10), create_leaf(11));
    // Node* root = create_op(2, min1, min2);

    // CONVERT TREE TO STACK ARRAY (ONLY OBTAINS # OF REGS!!!!)

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
    double* regArray = (double*)malloc(sizeof(double) * neededRegs);

    for (int i = 0; i <= theIndex; i++)
    {
        switch (stackArray[i].op_type)
        {
        case 0:
            double arg1, arg2;
            if (stackArray[i].arg_indexes[0] < 0) {
                arg1 = theStack->const_args[abs(stackArray[i].arg_indexes[0]) - 1];
                // printf("\n%lf", arg1);
            }
            else
                arg1 = regArray[stackArray[i].arg_indexes[0]];
            if (stackArray[i].arg_indexes[1] < 0) {
                arg2 = theStack->const_args[abs(stackArray[i].arg_indexes[1]) - 1];
                // printf("\n%lf", arg2);
            }
            else
                arg2 = regArray[stackArray[i].arg_indexes[1]];
            
            printf("\n%d", stackArray[i].reg_index);
            regArray[stackArray[i].reg_index] = arg1 + arg2;
            break;
        default:
            break;
        }
    }
    printf("\n%lf", regArray[0]);


    // int theMax = max(stackArray, theIndex) + 1;
    // printf("\n%d", theMax);

    // ── 4. Phase 1 ──────────────────────────────────────────────────────────
    // printf("=== Phase 1: Sethi-Ullman Labeling (original tree) ===\n");
    // compute_labels(root);
    // printf("Phase 1 complete...\n");
    // print_tree(root, 0);

    // ── 5. Tree-to-Vine ─────────────────────────────────────────────────────
    // printf("\n=== Tree-to-Vine Conversion ===\n");
    // root = tree_to_vine(root);
    // printf("Vine constructed. Recomputing labels...\n");
    // compute_labels(root);
    // print_tree(root, 0);

    // ── 6. Phase 2 ──────────────────────────────────────────────────────────
    // printf("\n=== Phase 2: Code Generation (post-vine) ===\n");
    // generate_code(root);

    // ── 7. Report ───────────────────────────────────────────────────────────
    // report_registers_used(pool_size);

    return 0;
}