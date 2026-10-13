#ifndef STACK_BUILDER_H
#define STACK_BUILDER_H

#include <stddef.h>
#include <stdint.h>
#include "op_stack.h"
#include "noise_gen.h"

typedef struct OpInfo {
    int             op_type;
    int             priority;      // higher number = tighter binding
    int             associative;   // 1 = safe to rotate, 0 = do not rotate
} OpInfo;

typedef struct EvalStack {
    StackFrame* op_stack;
    size_t op_stack_len;
    size_t max_reg;
    double* const_args;
    size_t const_args_len;
    NormalNoise* noise_states;
    size_t noise_states_len;
    ImprovedNoise* noise_levels;
    size_t noise_levels_len;
    uint32_t* interp_pc_indexes;
    size_t interp_len;
    size_t cache_once_len;
    size_t cache_flat_len;
    size_t cache_2d_len;
    uint32_t scale;
    int32_t min_y;
    int32_t height;
    uint32_t cell_size_x;
    uint32_t cell_size_y;
} EvalStack;

typedef struct EvalCache {
    int interp_enabled;
    int cache_once_enabled;
    int cache_flat_enabled;
    int cache_2d_enabled;
    int cell_width;
    int cell_height;
    int xz_samples; // Number of samples along the x or z axis
    int min_y;
    int num_interp_functions;
    double* nan_template;
    // Array dimensions: num z samples * num x samples * num interpolation functions
    double* interp;
    // Array dimensions: 4 * num interpolation functions
    double* active_cell_top;
    int cell_x;
    int cell_z;
    int num_cache_once_functions;
    // Array dimensions: chunk height * 17 * 17 * num cache once functions
    double* cache_once;
    int num_cache_flat_functions;
    // Array dimensions: num z samples * num x samples * num cache flat functions
    double* cache_flat;
    int num_cache_2d_functions;
    // Array dimensions: 17 * 17 * num cache 2d functions
    double* cache_2d;
    int temp_x;
    int temp_y;
    int temp_z;
} EvalCache;

typedef struct EvalStackAllocReqs {
    size_t op_stack_len;
    size_t const_args_len;
    size_t interp_pc_indexes_len;
} EvalStackAllocReqs;

typedef enum { NODE_LEAF, NODE_OP } NodeType;

typedef struct Node {
    NodeType        type;
    int             operationType;
    int (*generate_stack_recursive_from_op)(EvalStack* evalStack, size_t currentFrameIndex, struct Node* node, int regPos);
    int             label;
    int             depth;
    int             numChildren;
    double*         values;
    struct Node**   allChildren;
} Node;

int max_val(int *arr, int n); // returns the max from an array of n numbers
void register_op(int op_type, int priority, int associative); // registers a new operation
OpInfo* lookup_op(int op_type); // looks up an operation
EvalStack* generate_stack_wrapper(Node* node, EvalStack* evalStack); // wrapper function to generate evaluation stack
Node* create_leaf(int op_type, double* theData); // creates a leaf node with data
Node* create_op(int op_type, Node** theChildren, double* theData); // creates an operation node with children
int rotation_is_safe(Node* rest); // CLAUDE FUNCTION - determines whether or not a child is safe to rotate
void rotate_right(Node* tail); // CLAUDE FUNCTION - rotates a child node
int larger_arg_right(Node* root); // swap children with the largest args to the right
Node* tree_to_vine(Node* root); // CLAUDE FUNCTION - "reassociation" optimization, which smashes a tree into a linked list
static inline double get_arg(EvalStack* s, double* registers, int offset, int arg_num); // gets argument from const args or register (depending on op children)
static inline double get_const(EvalStack* s, int offset, int arg_num); // gets an op's constant args
double evaluate_stack(EvalStack* finalStack, int x, int y, int z, EvalCache* evalCache); // evaluates the stack given the inputs and coordinates
void fill_nan(double* arr, int len);
void advance_cell_x(EvalCache* cache);
void advance_cell_y(EvalCache* cache);
void advance_cell_z(EvalCache* cache);

#endif