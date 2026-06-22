#include <stdbool.h>
#include "op_types.h"
#include "op_stack.h"
#include "test_cases.h"
#include "stack_builder.h"

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

#define EPSILON 0.0000000000001

int test_full_algo(Node* root, char* case_name, int case_num, int expected_regs, double expected_result) {
    printf("-------------------------\n");
    printf("Test Case %d: %s\n", case_num, case_name);
    root = tree_to_vine(root);
    EvalStack* theStack = generate_stack_wrapper(root);
    double resultOfOps = eval_stack(theStack);
    char exp_regs[sizeof(int)*8+1];
    sprintf(exp_regs, ", Expected: %d\n", expected_regs);
    printf("Registers Used: %d%s", theStack->max_reg+1, expected_regs != -1 ? exp_regs : "\n");
    printf("Result: %lf, Expected: %lf\n", resultOfOps, expected_result);
    if (expected_regs != -1 && theStack->max_reg+1 != expected_regs) {
        if (fabs(resultOfOps - expected_result) > EPSILON) {
            return 3;
        }
        return 1;
    } else if (fabs(resultOfOps - expected_result) > EPSILON) {
        return 2;
    }
    return 0;
}

void run_all_test_cases() {
    TestCase cases[] = {
        { .name = "Larger Opt (Add Associativity + Larger Branch Reorder)", .fn = larger_opt_test_case, .exp_regs = 1, .exp_result = 76.0 },
        { .name = "Larger Opt Shifted Noise 1 (Child associativity)", .fn = larger_opt_test_case_shifted_noise1, .exp_regs = 1, .exp_result = 86.0 },
        { .name = "Larger Opt Shifted Noise 2 (Child non-associativity)", .fn = larger_opt_test_case_shifted_noise2, .exp_regs = 3, .exp_result = 48.0 },
        { .name = "One Arg", .fn = one_arg_test_case, .exp_regs = 2, .exp_result = 67.0 },
        { .name = "Min and Max", .fn = min_and_max_test_case, .exp_regs = 2, .exp_result = 67.0 },
        { .name = "Add/Mul Associativity", .fn = associativity_test_case, .exp_regs = 1, .exp_result = 9.0 },
        { .name = "Square", .fn = square_test_case, .exp_regs = 2, .exp_result = 20151121.0 },
        { .name = "Cube", .fn = cube_test_case, .exp_regs = 2, .exp_result = 90458382169.0 },
        { .name = "Half + Quarter Negative", .fn = half_quarter_test_case, .exp_regs = 2, .exp_result = -67.0 },
        { .name = "Inverse", .fn = inverse_test_case, .exp_regs = 1, .exp_result = 67.0 },
        { .name = "Squeeze", .fn = squeeze_test_case, .exp_regs = 1, .exp_result = 0.458333333333333333333333333333 },
        { .name = "Clamp", .fn = clamp_test_case, .exp_regs = 1, .exp_result = 1.0 },
    };
    bool consider_regs = true;
    int len = sizeof(cases) / sizeof(cases[0]);
    int err, total_passed = 0;
    for (int i = 0; i < len; i++) {
        err = test_full_algo(cases[i].fn(), cases[i].name, i+1, consider_regs ? cases[i].exp_regs : -1, cases[i].exp_result );
        if (err == 1) {
            printf("\x1B[31mFailed: Incorrect Register Usage\033[0m\n");
        } else if (err == 2) {
            printf("\x1B[31mFailed: Incorrect Result\033[0m\n");
        } else if (err == 3) {
            printf("\x1B[31mFailed: Incorrect Register Usage and Result\033[0m\n");
        } else {
            printf("\x1B[32mPassed\033[0m\n");
            total_passed++;
        }
    }
    printf("-------------------------\n");
    if (total_passed == len) {
        printf("\x1B[32mAll Tests Passed (%d Passed / %d Total)\033[0m\n", total_passed, len);
    } else {
        printf("\x1B[31mTest Failed (%d Passed / %d Total)\033[0m\n", total_passed, len);
    }
}