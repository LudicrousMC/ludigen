#include <stdbool.h>
#include "op_types.h"
#include "op_stack.h"
#include "test_cases.h"

Node* larger_opt_test_case() {
    Node* add1 = create_op(OP_ADD, (Node *[]){create_leaf(OP_CONST, 8), create_leaf(OP_CONST, 9)});
    Node* add2 = create_op(OP_ADD, (Node *[]){create_leaf(OP_CONST, 10), create_leaf(OP_CONST, 11)});
    Node* add3 = create_op(OP_ADD, (Node *[]){create_leaf(OP_CONST, 6), create_leaf(OP_CONST, 7)});
    Node* add4 = create_op(OP_ADD, (Node *[]){add2, add3});
    Node* add5 = create_op(OP_ADD, (Node *[]){create_leaf(OP_CONST, 12), create_leaf(OP_CONST, 13)});
    Node* add6 = create_op(OP_ADD, (Node *[]){add4, add5});
    Node* root = create_op(OP_ADD, (Node *[]){add1, add6});
    return root;
}

Node* larger_opt_test_case_shifted_noise1() {
    Node* add1 = create_op(OP_ADD, (Node *[]){create_leaf(OP_CONST, 8), create_leaf(OP_CONST, 9)});
    Node* add2 = create_op(OP_SHIFTED_NOISE, (Node *[]){create_leaf(OP_CONST, 10), create_leaf(OP_CONST, 11), create_leaf(OP_CONST, 10)});
    Node* add3 = create_op(OP_ADD, (Node *[]){create_leaf(OP_CONST, 6), create_leaf(OP_CONST, 7)});
    Node* add4 = create_op(OP_ADD, (Node *[]){add2, add3});
    Node* add5 = create_op(OP_ADD, (Node *[]){create_leaf(OP_CONST, 12), create_leaf(OP_CONST, 13)});
    Node* add6 = create_op(OP_ADD, (Node *[]){add4, add5});
    Node* root = create_op(OP_ADD, (Node *[]){add1, add6});
    return root;
}

Node* larger_opt_test_case_shifted_noise2() {
    Node* add0 = create_op(OP_ADD, (Node *[]){create_leaf(OP_CONST, 1), create_leaf(OP_CONST, 2)});
    Node* add3 = create_op(OP_ADD, (Node *[]){create_leaf(OP_CONST, 2), create_leaf(OP_CONST, 3)});
    Node* add4 = create_op(OP_ADD, (Node *[]){create_leaf(OP_CONST, 3), create_leaf(OP_CONST, 4)});
    Node* add1 = create_op(OP_ADD, (Node *[]){add3, add4});
    Node* add7 = create_op(OP_ADD, (Node *[]){create_leaf(OP_CONST, 4), create_leaf(OP_CONST, 5)});
    Node* add8 = create_op(OP_ADD, (Node *[]){create_leaf(OP_CONST, 5), create_leaf(OP_CONST, 6)});
    Node* add5 = create_op(OP_ADD, (Node *[]){add7, add8});
    Node* add6 = create_op(OP_ADD, (Node *[]){create_leaf(OP_CONST, 6), create_leaf(OP_CONST, 7)});
    Node* add2 = create_op(OP_ADD, (Node *[]){add5, add6});
    
    Node* root = create_op(OP_SHIFTED_NOISE, (Node *[]){add0, add1, add2});
    return root;
}

Node* one_arg_test_case() {
    Node* abs1 = create_op(OP_ABS, (Node *[]){create_leaf(OP_CONST, -60)});
    Node* abs2 = create_op(OP_ABS, (Node *[]){create_leaf(OP_CONST, -7)});
    Node* root = create_op(OP_ADD, (Node *[]){abs1, abs2});
    return root;
}

Node* min_and_max_test_case() { // FIX THIS, YOU ABSOLUTE MIFFLER, YOU ABSOSOLUTE BELLEND
    Node* abs1 = create_op(OP_MAX, (Node *[]){create_leaf(OP_CONST, 60), create_leaf(OP_CONST, 6.7)});
    Node* abs2 = create_op(OP_MIN, (Node *[]){create_leaf(OP_CONST, 66.7), create_leaf(OP_CONST, 7)});
    Node* root = create_op(OP_ADD, (Node *[]){abs1, abs2});
    return root;
}

Node* associativity_test_case() {
    Node* add = create_op(OP_ADD, (Node *[]){create_leaf(OP_CONST, 1), create_leaf(OP_CONST, 2)});
    Node* root = create_op(OP_MUL, (Node *[]){add, create_leaf(OP_CONST, 3)});
    return root;
}

Node* square_test_case() {
    Node* add = create_op(OP_ADD, (Node *[]){create_leaf(OP_CONST, 60), create_leaf(OP_CONST, 7)});
    Node* mult = create_op(OP_MUL, (Node *[]){create_leaf(OP_CONST, 67), create_leaf(OP_CONST, 1)});
    Node* squareAdd = create_op(OP_SQUARE, (Node *[]){add});
    Node* squareMult = create_op(OP_SQUARE, (Node *[]){mult});
    Node* root = create_op(OP_MUL, (Node *[]){squareAdd, squareMult});
    return root;
}

Node* cube_test_case() {
    Node* add = create_op(OP_ADD, (Node *[]){create_leaf(OP_CONST, 60), create_leaf(OP_CONST, 7)});
    Node* mult = create_op(OP_MUL, (Node *[]){create_leaf(OP_CONST, 67), create_leaf(OP_CONST, 1)});
    Node* squareAdd = create_op(OP_CUBE, (Node *[]){add});
    Node* squareMult = create_op(OP_CUBE, (Node *[]){mult});
    Node* root = create_op(OP_MUL, (Node *[]){squareAdd, squareMult});
    return root;
}

Node* half_quarter_test_case() {
    Node* add = create_op(OP_ADD, (Node *[]){create_leaf(OP_CONST, -33.5), create_leaf(OP_CONST, -33.5)});
    Node* mult = create_op(OP_MUL, (Node *[]){create_leaf(OP_CONST, 67), create_leaf(OP_CONST, -2)});
    Node* squareAdd = create_op(OP_HALF_NEGATIVE, (Node *[]){add});
    Node* squareMult = create_op(OP_QUARTER_NEGATIVE, (Node *[]){mult});
    Node* root = create_op(OP_ADD, (Node *[]){squareAdd, squareMult});
    return root;
}

Node* inverse_test_case() {
    return create_op(OP_INVERT, (Node *[]){create_leaf(OP_CONST, 0.0149253731343283582089552238806)});
}

Node* squeeze_test_case() {
    return create_op(OP_SQUEEZE, (Node *[]){create_leaf(OP_CONST, 67)});
}

Node* clamp_test_case() {
    return create_op(OP_CLAMP, (Node *[]){create_leaf(OP_CONST, 67)});
}