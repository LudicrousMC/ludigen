#ifndef TEST_CASES_H
#define TEST_CASES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "op_types.h"
#include "op_stack.h"
#include "stack_builder.h"

Node* larger_opt_test_case(); // generates a test case for larger branch last optimization: 1 reg used
Node* larger_opt_test_case_shifted_noise1(); // generates a test case for larger branch last optimization with shifted noise: 1 regs used
Node* larger_opt_test_case_shifted_noise2(); // generates a test case for larger branch last optimization with shifted noise: 3 regs used
Node* one_arg_test_case(); // generates a test case for one arg ops: 2 regs used
Node* min_and_max_test_case(); // generates a test case for min and max: 2 regs used
Node* associativity_test_case(); // // generates a test case for associativity: 2 regs used

#endif