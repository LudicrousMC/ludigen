#ifndef TEST_CASES_H
#define TEST_CASES_H

#include "op_types.h"
#include "op_stack.h"
#include "stack_builder.h"

typedef struct {
    char* name;
    Node* (*fn)(void);
    int exp_regs;
    double exp_result;
} TestCase;

Node* larger_opt_test_case(); // generates a test case for larger branch last optimization: 1 reg used
Node* larger_opt_test_case_shifted_noise1(); // generates a test case for larger branch last optimization with shifted noise: 1 regs used
Node* larger_opt_test_case_shifted_noise2(); // generates a test case for larger branch last optimization with shifted noise: 3 regs used
Node* one_arg_test_case(); // generates a test case for one arg ops: 2 regs used
Node* min_and_max_test_case(); // generates a test case for min and max: 2 regs used
Node* associativity_test_case(); // // generates a test case for associativity: 2 regs used
Node* square_test_case(); // generates a test case for square: 2 regs used
Node* cube_test_case(); // generates a test case for cube: 2 regs used
Node* half_quarter_test_case(); // generates a test case for half negative and quarter negative: 2 regs used
Node* inverse_test_case(); // generates a test case for inverse: 1 reg used
Node* squeeze_test_case(); // generates a test case for squeeze: 1 reg used
Node* clamp_test_case(); // generates a test case for clamp: 1 reg used
Node* y_clamped_grad_test_case(); // generates a test case for y clamped gradient: 1 reg used
int test_full_algo(Node* root, char* case_name, int case_num, int expected_regs, double expected_result);
void run_all_test_cases();

#endif