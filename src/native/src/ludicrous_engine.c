#include <vulkan/vulkan.h>
#include <jni.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ludicrous_engine.h"
#include "op_stack.h"
#include "op_types.h"
#include "vulkan_util.h"
#include "shader.h"
#include "logger.h"
#include "yyjson.h"
#include "stack_builder.h"
#include "noise_gen.h"

#define VULKAN_VERSION VK_API_VERSION_1_0
#define ELEMENT_COUNT 1024
#define BUFFER_SIZE (ELEMENT_COUNT * sizeof(double))

// Usage: InstanceState* state = create_instance(VK_QUEUE_COMPUTE_BIT);
static InstanceState* create_instance(int queueFamilyType) {

    // <-- Setup Vulkan Instance -->
    // Create Vulkan instance with Vulkan API version 1.0
    VkInstance instance;
    VkResult err = vkCreateInstance(&(VkInstanceCreateInfo){
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &(VkApplicationInfo){
                .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                .apiVersion = VULKAN_VERSION,
            }
        }, NULL, &instance);
    if (err != VK_SUCCESS)
        return NULL;

    // <-- Get Physical Device -->
    // Query Physical devices (pick first)
    VkPhysicalDevice physDev;
    err = vkEnumeratePhysicalDevices(instance, &(uint32_t){1}, &physDev);
    if (err < VK_SUCCESS)
        return NULL;

    // <-- Find Index of Queue Family Type in Physical Device -->
    uint32_t qfCount;
    // Get number of queue family properties
    vkGetPhysicalDeviceQueueFamilyProperties(physDev, &qfCount, NULL);
    // Allocate and set queue family props
    VkQueueFamilyProperties *qfProps = malloc(qfCount * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(physDev, &qfCount, qfProps);
    // Find first occurrence of a Queue Family that supports specified family type
    uint32_t queueFamilyIndex = UINT32_MAX;
    for (uint32_t i = 0; i < qfCount; i++) {
        if (qfProps[i].queueFlags & queueFamilyType) {
            queueFamilyIndex = i;
            break;
        }
    }
    free(qfProps);

    // <-- Create Logical Device and Queue with Queue Family -->
    VkDevice device;
    err = vkCreateDevice(physDev, &(VkDeviceCreateInfo){
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &(VkDeviceQueueCreateInfo){
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = queueFamilyIndex,
                .queueCount = 1,
                .pQueuePriorities = &(float){1.0f},
            }
        }, NULL, &device);
    if (err != VK_SUCCESS)
        return NULL;

    // <-- Get Logical Device's Created Queue -->
    VkQueue queue;
    vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);

    InstanceState* state = (InstanceState*)malloc(sizeof(InstanceState));
    *state = (InstanceState){
        .vk_instance = instance,
        .vk_phys_device = physDev,
        .vk_queue_family_index = queueFamilyIndex,
        .vk_device = device,
        .vk_queue = queue,
        .shaders = malloc(0),
        .shaders_len = 0,
    };
    return state;
}

int string_to_op_type(const char* str) {
    if (strcmp(str, "minecraft:add") == 0) {
        return OP_ADD;
    } else if (strcmp(str, "minecraft:mul") == 0) {
        return OP_MUL;
    } else if (strcmp(str, "minecraft:min") == 0) {
        return OP_MIN;
    } else if (strcmp(str, "minecraft:max") == 0) {
        return OP_MAX;
    } else if (strcmp(str, "minecraft:abs") == 0) {
        return OP_ABS;
    } else if (strcmp(str, "minecraft:square") == 0) {
        return OP_SQUARE;
    } else if (strcmp(str, "minecraft:cube") == 0) {
        return OP_CUBE;
    } else if (strcmp(str, "minecraft:half_negative") == 0) {
        return OP_HALF_NEGATIVE;
    } else if (strcmp(str, "minecraft:quarter_negative") == 0) {
        return OP_QUARTER_NEGATIVE;
    } else if (strcmp(str, "minecraft:invert") == 0) {
        return OP_INVERT;
    } else if (strcmp(str, "minecraft:squeeze") == 0) {
        return OP_SQUEEZE;
    } else if (strcmp(str, "minecraft:clamp") == 0) {
        return OP_CLAMP;
    } else if (strcmp(str, "minecraft:y_clamped_gradient") == 0) {
        return OP_Y_CLAMPED_GRAD;
    } else if (strcmp(str, "minecraft:find_top_surface") == 0) {
        return OP_FIND_TOP_SURFACE;
    } else if (strcmp(str, "minecraft:range_choice") == 0) {
        return OP_RANGE_CHOICE;
    } else if (strcmp(str, "minecraft:interval_select") == 0) {
        return OP_INTERVAL_SELECT;
    } else if (strcmp(str, "minecraft:noise") == 0) {
        return OP_NOISE;
    } else if (strcmp(str, "minecraft:shifted_noise") == 0) {
        return OP_SHIFTED_NOISE;
    } else if (strcmp(str, "minecraft:spline") == 0) {
        return OP_SPLINE;
    } else if (strcmp(str, "minecraft:blend_density") == 0) {
        return OP_BLEND_DENSITY;
    } else if (strcmp(str, "minecraft:blend_offset") == 0) {
        return OP_BLEND_OFFSET;
    } else if (strcmp(str, "minecraft:blend_alpha") == 0) {
        return OP_BLEND_ALPHA;
    } else if (strcmp(str, "minecraft:interpolated") == 0) {
        return OP_INTERPOLATED;
    } else if (strcmp(str, "minecraft:cache_once") == 0) {
        return OP_CACHE_ONCE;
    } else if (strcmp(str, "minecraft:flat_cache") == 0) {
        return OP_FLAT_CACHE;
    } else if (strcmp(str, "minecraft:cache_2d") == 0) {
        return OP_CACHE_2D;
    } else if (strcmp(str, "minecraft:cache_all_in_cell") == 0) {
        return OP_CACHE_ALL_IN_CELL;
    } else if (strcmp(str, "minecraft:shift_a") == 0) {
        return OP_SHIFT_A;
    } else if (strcmp(str, "minecraft:shift_b") == 0) {
        return OP_SHIFT_B;
    } else if (strcmp(str, "minecraft:old_blended_noise") == 0) {
        return OP_OLD_BLENDED_NOISE;
    } else if (strcmp(str, "minecraft:end_islands") == 0) {
        return OP_END_ISLANDS;
    }
}

Node* build_spline_from_json_recursive(yyjson_val* json) {
    uint8_t json_type = yyjson_get_type(json);
    if (json_type == YYJSON_TYPE_NUM) {
        return create_leaf(OP_CONST, (double[]){yyjson_get_num(json)});
    }

    Node* coordinate = build_tree_from_json_recursive(yyjson_obj_get(json, "coordinate"));
    size_t num_points = yyjson_arr_size(yyjson_obj_get(json, "points"));
    Node* points[num_points + 1];
    points[num_points] = coordinate;
    yyjson_arr_iter iter = yyjson_arr_iter_with(yyjson_obj_get(json, "points"));
    yyjson_val* val;
    for (int i = 0; (val = yyjson_arr_iter_next(&iter)); i++) {
        points[i] = create_op(OP_SPLINE,
            (Node*[]){build_spline_from_json_recursive(yyjson_obj_get(val, "value"))},
            (double[]){
                OP_SUBTYPE_SPLINE_POINT,
                yyjson_get_num(yyjson_obj_get(val, "location")),
                yyjson_get_num(yyjson_obj_get(val, "derivative"))
            }
        );
    }
    return create_op(OP_SPLINE, points, (double[]){OP_SUBTYPE_SPLINE, num_points});
}

Node* build_tree_from_json_recursive(yyjson_val* json) {
    uint8_t json_type = yyjson_get_type(json);
    // If json is constant number, return new constant node
    if (json_type == YYJSON_TYPE_NUM) {
        return create_leaf(OP_CONST, (double[]){yyjson_get_num(json)});
    }

    int op_type = string_to_op_type(yyjson_get_str(yyjson_obj_get(json, "type")));

    // Two Arg Operations
    if (op_type == OP_ADD ||
        op_type == OP_MUL ||
        op_type == OP_MIN ||
        op_type == OP_MAX)
    {
        Node* arg1 = build_tree_from_json_recursive(yyjson_obj_get(json, "argument1"));
        Node* arg2 = build_tree_from_json_recursive(yyjson_obj_get(json, "argument2"));
        return create_op(op_type, (Node*[]){arg1, arg2}, NULL);
    }
    // One Arg Operations
    if (op_type == OP_ABS ||
        op_type == OP_SQUARE ||
        op_type == OP_CUBE ||
        op_type == OP_HALF_NEGATIVE ||
        op_type == OP_QUARTER_NEGATIVE ||
        op_type == OP_INVERT ||
        op_type == OP_SQUEEZE)
    {
        Node* arg1 = build_tree_from_json_recursive(yyjson_obj_get(json, "argument"));
        return create_op(op_type, &arg1, NULL);
    }
    if (op_type == OP_CLAMP) {
        Node* input = build_tree_from_json_recursive(yyjson_obj_get(json, "input"));
        double min = yyjson_get_num(yyjson_obj_get(json, "min"));
        double max = yyjson_get_num(yyjson_obj_get(json, "max"));
        return create_op(OP_CLAMP, (Node*[]){input}, (double[]){min, max});
    }
    if (op_type == OP_Y_CLAMPED_GRAD) {
        int from_y = yyjson_get_int(yyjson_obj_get(json, "from_y"));
        int to_y = yyjson_get_int(yyjson_obj_get(json, "to_y"));
        double from_value = yyjson_get_num(yyjson_obj_get(json, "from_value"));
        double to_value = yyjson_get_num(yyjson_obj_get(json, "to_value"));
        return create_leaf(OP_Y_CLAMPED_GRAD, (double[]){(double)from_y, (double)to_y, from_value, to_value});
    }
    if (op_type == OP_FIND_TOP_SURFACE) {
        int cell_height = yyjson_get_int(yyjson_obj_get(json, "cell_height"));
        int lower_bound = yyjson_get_int(yyjson_obj_get(json, "lower_bound"));
        Node* upper_bound = build_tree_from_json_recursive(yyjson_obj_get(json, "upper_bound"));
        Node* density = build_tree_from_json_recursive(yyjson_obj_get(json, "density"));
        return create_op(OP_FIND_TOP_SURFACE, (Node*[]){upper_bound, density}, (double[]){cell_height, lower_bound});
    }
    if (op_type == OP_RANGE_CHOICE) {
        double min_inclusive = yyjson_get_num(yyjson_obj_get(json, "min_inclusive"));
        double max_exclusive = yyjson_get_num(yyjson_obj_get(json, "max_exclusive"));
        Node* input = build_tree_from_json_recursive(yyjson_obj_get(json, "input"));
        Node* when_in_range = build_tree_from_json_recursive(yyjson_obj_get(json, "when_in_range"));
        Node* when_out_of_range = build_tree_from_json_recursive(yyjson_obj_get(json, "when_out_of_range"));
        return create_op(OP_RANGE_CHOICE, (Node*[]){input, when_in_range, when_out_of_range}, (double[]){min_inclusive, max_exclusive});
    }
    if (op_type == OP_INTERVAL_SELECT) {
        size_t len = yyjson_arr_size(yyjson_obj_get(json, "thresholds"));
        double thresholds[len + 1];
        thresholds[0] = (double)len; // Number of thresholds
        Node* functions[len + 2]; // functions + input function
        functions[0] = build_tree_from_json_recursive(yyjson_obj_get(json, "input")); // Input function
        yyjson_arr_iter iter = yyjson_arr_iter_with(yyjson_obj_get(json, "thresholds"));
        yyjson_val* val;
        for (int i = 1; (val = yyjson_arr_iter_next(&iter)); i++) {
            thresholds[i] = yyjson_get_num(val);
        }
        iter = yyjson_arr_iter_with(yyjson_obj_get(json, "functions"));
        for (int i = 1; (val = yyjson_arr_iter_next(&iter)); i++) {
            functions[i] = build_tree_from_json_recursive(val);
        }

        return create_op(OP_INTERVAL_SELECT, functions, thresholds);
    }
    if (op_type == OP_NOISE) {
        int noise_index = yyjson_get_int(yyjson_obj_get(json, "noise"));
        double xz_scale = yyjson_get_num(yyjson_obj_get(json, "xz_scale"));
        double y_scale = yyjson_get_num(yyjson_obj_get(json, "y_scale"));
        return create_leaf(OP_NOISE, (double[]){(double)noise_index, xz_scale, y_scale});
    }
    if (op_type == OP_SHIFTED_NOISE) {
        int noise_index = yyjson_get_int(yyjson_obj_get(json, "noise"));
        double xz_scale = yyjson_get_num(yyjson_obj_get(json, "xz_scale"));
        double y_scale = yyjson_get_num(yyjson_obj_get(json, "y_scale"));
        Node* shift_x = build_tree_from_json_recursive(yyjson_obj_get(json, "shift_x"));
        Node* shift_y = build_tree_from_json_recursive(yyjson_obj_get(json, "shift_y"));
        Node* shift_z = build_tree_from_json_recursive(yyjson_obj_get(json, "shift_z"));
        return create_op(OP_SHIFTED_NOISE, (Node*[]){shift_x, shift_y, shift_z}, (double[]){noise_index, xz_scale, y_scale});
    }
    if (op_type == OP_SPLINE) {
        yyjson_val* spline_root = yyjson_obj_get(json, "spline");
        return build_spline_from_json_recursive(spline_root);
    }
    // Skips operations
    if (op_type == OP_BLEND_DENSITY ||
        op_type == OP_INTERPOLATED ||
        op_type == OP_CACHE_ONCE ||
        op_type == OP_FLAT_CACHE ||
        op_type == OP_CACHE_2D ||
        op_type == OP_CACHE_ALL_IN_CELL)
    {
        return build_tree_from_json_recursive(yyjson_obj_get(json, "argument"));
    }
    if (op_type == OP_BLEND_OFFSET || op_type == OP_BLEND_ALPHA || op_type == OP_END_ISLANDS) {
        return create_leaf(op_type, NULL);
    }
    if (op_type == OP_SHIFT_A || op_type == OP_SHIFT_B) {
        return create_leaf(op_type, (double[]){yyjson_get_int(yyjson_obj_get(json, "argument"))});
    }
    if (op_type == OP_OLD_BLENDED_NOISE) {
        double smear_scale_mult = yyjson_get_num(yyjson_obj_get(json, "smear_scale_multiplier"));
        double xz_factor = yyjson_get_num(yyjson_obj_get(json, "xz_factor"));
        double xz_scale = yyjson_get_num(yyjson_obj_get(json, "xz_scale"));
        double y_factor = yyjson_get_num(yyjson_obj_get(json, "y_factor"));
        double y_scale = yyjson_get_num(yyjson_obj_get(json, "y_scale"));
        return create_leaf(OP_OLD_BLENDED_NOISE, (double[]){smear_scale_mult, xz_factor, xz_scale, y_factor, y_scale});
    }
}

int main() {
    register_op(OP_ADD, 2, 1);
    register_op(OP_MUL, 3, 1);
    register_op(OP_MIN, 1, 1);
    register_op(OP_MAX, 1, 1);
    /*register_op(OP_ABS, 0, 0);
    register_op(OP_SQUARE, 0, 0);
    register_op(OP_CUBE, 0,0);
    register_op(OP_HALF_NEGATIVE, 0, 0);
    register_op(OP_QUARTER_NEGATIVE, 0, 0);
    register_op(OP_INVERT, 0, 0);
    register_op(OP_SQUEEZE, 0, 0);
    register_op(OP_CLAMP, 0, 0);
    register_op(OP_SHIFTED_NOISE, 10, 0);*/
    //const char* json = "{\"type\":\"minecraft:mul\",\"argument1\":{\"type\":\"minecraft:abs\",\"argument\":-0.1},\"argument2\":{\"type\":\"minecraft:add\",\"argument1\":0.05,\"argument2\":0.05}}";
    const char* json = "{\"type\":\"minecraft:spline\",\"spline\":{\"coordinate\":{\"type\":\"minecraft:add\",\"argument1\":1.0,\"argument2\":0.5},\"points\":[{\"derivative\":0.0,\"location\":0.5,\"value\":-7.0},{\"derivative\":0.0,\"location\":1.0,\"value\":{\"coordinate\":{\"type\":\"minecraft:abs\",\"argument\":-6.5},\"points\":[{\"derivative\":0.0,\"location\":6.0,\"value\":65.0},{\"derivative\":0.0,\"location\":7.0,\"value\":67.0}]}},{\"derivative\":0.0,\"location\":2.0,\"value\":{\"coordinate\":{\"type\":\"minecraft:mul\",\"argument1\":-4.0,\"argument2\":0.76},\"points\":[{\"derivative\":0.0,\"location\":-3.0,\"value\":68.0},{\"derivative\":0.0,\"location\":-1.0,\"value\":-70}]}}]}}";
    yyjson_doc* doc = yyjson_read(json, strlen(json), 0);
    yyjson_val* json_root = yyjson_doc_get_root(doc);

    Node* root = build_tree_from_json_recursive(json_root);

    root = tree_to_vine(root);
    EvalStack* stack = generate_stack_wrapper(root);
    double result = evaluate_stack(stack, 0, 0, 0);
    printf("registers: %d\n", stack->max_reg+1);
    printf("result: %.30lf\n", result);

    yyjson_doc_free(doc);
}

typedef struct {
    StackFrame data[1024];
} StackBuffer;

typedef struct {
    double data[512];
} ConstArgsBuffer;

/*int main() {
    InstanceState* state = create_instance(VK_QUEUE_COMPUTE_BIT);
    FILE *f = fopen("compute/eval.spv", "rb");
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    rewind(f);
    uint32_t *code = malloc(size);
    fread(code, 1, size, f);
    fclose(f);
    double const_args[1024] = {0};
    const_args[1] = 67.67;
    ShaderBufferData buffers[] = {
        { .buf = const_args, .size = sizeof(double) * 1024,
          .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
          .flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT},
        { .buf = NULL, .size = sizeof(double) * 1024,
          .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
          .flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
        { .buf = NULL, .size = sizeof(double) * 1024, .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
          .flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT }
    };
    VkDescriptorSetLayoutBinding desc_set_layout[] = {
        { .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
        { .binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
    };
    ShaderDescriptorLayout desc_layouts[] = {
        { .bindings = desc_set_layout,
          .num_bindings = sizeof(desc_set_layout) / sizeof(desc_set_layout[0]) },
    };
    ShaderDescriptorSetUpdate desc_updates[] = {
        { .desc_index = 0, .mem_indexes = (size_t[]){1}, .buf_indexes = (size_t[]){0},
          .desc_count = 1, .set_index = 0 },
        { .desc_index = 1, .mem_indexes = (size_t[]){2}, .buf_indexes = (size_t[]){0},
          .desc_count = 1, .set_index = 0 },
    };
    ShaderCreateData data = {
        .buffers = buffers,
        .num_buffers = sizeof(buffers) / sizeof(buffers[0]),
        .desc_layouts = desc_layouts,
        .num_desc_layouts = sizeof(desc_layouts) / sizeof(desc_layouts[0]),
        .desc_updates = desc_updates,
        .num_desc_updates = sizeof(desc_updates) / sizeof(desc_updates[0]),
    };
    ShaderData shader;
    create_shader(state, code, size, &data, &shader);
    // Start recording for command buffer
    vkBeginCommandBuffer(shader.vk_cmd, &(VkCommandBufferBeginInfo){
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    });
    vkCmdCopyBuffer(shader.vk_cmd, shader.shader_mem[0].vk_buf[0], shader.shader_mem[1].vk_buf[0], 1, &(VkBufferCopy){ .size = sizeof(double) * 1024 });
    // Bind compute pipeline to command buffer
    vkCmdBindPipeline(shader.vk_cmd, VK_PIPELINE_BIND_POINT_COMPUTE, shader.vk_pipeline);
    // Bind compute pipeline descriptors to command buffer
    vkCmdBindDescriptorSets(shader.vk_cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                             shader.vk_pipeline_layout, 0, shader.vk_desc_sets_len, shader.vk_desc_sets, 0, NULL);
    // local_size_x=64, so groups = ELEMENT_COUNT / 64
    // Set the dispatched group sizes
    vkCmdDispatch(shader.vk_cmd, ELEMENT_COUNT / 64, 1, 1);
    // Stop recording command buffer
    vkEndCommandBuffer(shader.vk_cmd);

    // Create wait barrier
    VkFence fence;
    vkCreateFence(state->vk_device,
        &(VkFenceCreateInfo){
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        }, NULL, &fence);
    // Execute compute pipeline via command buffer
    vkQueueSubmit(state->vk_queue, 1, &(VkSubmitInfo){
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &(shader.vk_cmd),
        }, fence);
    // Wait for barrier complete signal
    vkWaitForFences(state->vk_device, 1, &fence, VK_TRUE, UINT64_MAX);

    // Read results
    double *results;
    vkMapMemory(state->vk_device, shader.shader_mem[2].vk_mem, 0, BUFFER_SIZE, 0, (void **)&results);
    printf("out[0]=%f  out[1]=%f  out[1023]=%f\n",
           results[0], results[1], results[1023]);
    vkUnmapMemory(state->vk_device, shader.shader_mem[2].vk_mem);
    return 0;
}*/

int vulkan_test(uint32_t* spirv, size_t size) {
    // Create Vulkan instance with Vulkan API version 1.0
    VkInstance instance;
    vkCreateInstance(&(VkInstanceCreateInfo){
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &(VkApplicationInfo){
                .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                .apiVersion = VK_API_VERSION_1_0,
            },
        }, NULL, &instance);

    // Get Physical device (pick first)
    //uint32_t devCount = 1;
    VkPhysicalDevice physDev;
    vkEnumeratePhysicalDevices(instance, &(uint32_t){1}, &physDev);

    // Find a compute queue family
    uint32_t qfCount;
    // Get number of queue family properties
    vkGetPhysicalDeviceQueueFamilyProperties(physDev, &qfCount, NULL);
    // Allocate and set queue family props
    VkQueueFamilyProperties *qfProps = malloc(qfCount * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(physDev, &qfCount, qfProps);
    // Find first occurrence of a Queue Family that supports compute operations
    uint32_t queueFamilyIndex = UINT32_MAX;
    for (uint32_t i = 0; i < qfCount; i++) {
        if (qfProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            queueFamilyIndex = i;
            break;
        }
    }
    free(qfProps);

    // Create Logical device with queue family
    VkDevice device;
    vkCreateDevice(physDev, &(VkDeviceCreateInfo){
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &(VkDeviceQueueCreateInfo){
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = queueFamilyIndex,
                .queueCount = 1,
                .pQueuePriorities = &(float){1.0f},
            }
        }, NULL, &device);

    // Get device queue
    VkQueue queue;
    vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);

    // Prepare host/cpu visible buffers
    VkBuffer stackBuf, constArgsBuf, outBuf;
    VkDeviceMemory stackMem, constArgsMem, outMem;
    VkMemoryPropertyFlags hostVisible =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    // Create stack buffer
    createBuffer(device, physDev, sizeof(StackBuffer),
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, hostVisible,
                 &stackBuf, &stackMem);
    // Create const args buffer
    createBuffer(device, physDev, sizeof(ConstArgsBuffer),
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, hostVisible,
                 &constArgsBuf, &constArgsMem);
    createBuffer(device, physDev, BUFFER_SIZE,
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, hostVisible,
                 &outBuf, &outMem);

    // 6. Upload input data
    void *mapped1;
    StackBuffer stack = {0};
    vkMapMemory(device, stackMem, 0, sizeof(StackBuffer), 0, &mapped1);
    memcpy(mapped1, &stack, sizeof(StackBuffer));
    vkUnmapMemory(device, stackMem);
    void *mapped2;
    ConstArgsBuffer const_args = {0};
    const_args.data[0] = 67.67f;
    vkMapMemory(device, constArgsMem, 0, sizeof(ConstArgsBuffer), 0, &mapped2);
    memcpy(mapped2, &const_args, sizeof(ConstArgsBuffer));
    vkUnmapMemory(device, constArgsMem);

    // 7. Descriptor set layout
    VkDescriptorSetLayoutBinding bindings[3] = {
        { .binding=0, .descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .descriptorCount=1, .stageFlags=VK_SHADER_STAGE_COMPUTE_BIT },
        { .binding=1, .descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .descriptorCount=1, .stageFlags=VK_SHADER_STAGE_COMPUTE_BIT },
        { .binding=2, .descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .descriptorCount=1, .stageFlags=VK_SHADER_STAGE_COMPUTE_BIT },
    };
    VkDescriptorSetLayoutCreateInfo dslCI = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 3,
        .pBindings    = bindings,
    };
    VkDescriptorSetLayout descLayout;
    vkCreateDescriptorSetLayout(device, &dslCI, NULL, &descLayout);

    // Pipeline layout
    VkPipelineLayoutCreateInfo plCI = {
        .sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts    = &descLayout,
    };
    VkPipelineLayout pipelineLayout;
    vkCreatePipelineLayout(device, &plCI, NULL, &pipelineLayout);

    // Create Compute Pipeline
    VkShaderModule shaderMod = createShaderModuleFromBytes(device, spirv, size);
    VkComputePipelineCreateInfo cpCI = {
        .sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage  = {
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = shaderMod,
            .pName  = "main",
        },
        .layout = pipelineLayout,
    };
    VkPipeline pipeline;
    vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &cpCI, NULL, &pipeline);

    // 9. Descriptor pool + set
    VkDescriptorPoolSize poolSize = {
        .type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 3,
    };
    VkDescriptorPoolCreateInfo dpCI = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets       = 1,
        .poolSizeCount = 1,
        .pPoolSizes    = &poolSize,
    };
    VkDescriptorPool descPool;
    vkCreateDescriptorPool(device, &dpCI, NULL, &descPool);

    VkDescriptorSetAllocateInfo dsAI = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = descPool,
        .descriptorSetCount = 1,
        .pSetLayouts        = &descLayout,
    };
    VkDescriptorSet descSet;
    vkAllocateDescriptorSets(device, &dsAI, &descSet);

    // 10. Write descriptors
    VkDescriptorBufferInfo stackInfo  = { stackBuf,  0, sizeof(StackBuffer) };
    VkDescriptorBufferInfo constArgsInfo  = { constArgsBuf,  0, sizeof(ConstArgsBuffer) };
    VkDescriptorBufferInfo outInfo = { outBuf, 0, BUFFER_SIZE };
    VkWriteDescriptorSet writes[3] = {
        { .sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet=descSet,
          .dstBinding=0, .descriptorCount=1,
          .descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .pBufferInfo=&stackInfo },
        { .sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet=descSet,
          .dstBinding=1, .descriptorCount=1,
          .descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .pBufferInfo=&constArgsInfo },
        { .sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet=descSet,
          .dstBinding=2, .descriptorCount=1,
          .descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .pBufferInfo=&outInfo },
    };
    vkUpdateDescriptorSets(device, 3, writes, 0, NULL);

    // Create Command buffer pool
    VkCommandPoolCreateInfo cpoolCI = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = queueFamilyIndex,
    };
    VkCommandPool cmdPool;
    vkCreateCommandPool(device, &cpoolCI, NULL, &cmdPool);

    VkCommandBufferAllocateInfo cbAI = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = cmdPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    // Allocate command buffer
    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(device, &cbAI, &cmd);

    // Start recording for command buffer
    vkBeginCommandBuffer(cmd, &(VkCommandBufferBeginInfo){
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    });
    // Bind compute pipeline to command buffer
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    // Bind compute pipeline descriptors to command buffer
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                             pipelineLayout, 0, 1, &descSet, 0, NULL);
    // local_size_x=64, so groups = ELEMENT_COUNT / 64
    // Set the dispatched group sizes
    vkCmdDispatch(cmd, ELEMENT_COUNT / 64, 1, 1);
    // Stop recording command buffer
    vkEndCommandBuffer(cmd);

    // Create wait barrier
    VkFence fence;
    vkCreateFence(device,
        &(VkFenceCreateInfo){
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        }, NULL, &fence);
    // Execute compute pipeline via command buffer
    vkQueueSubmit(queue, 1, &(VkSubmitInfo){
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &cmd,
        }, fence);
    // Wait for barrier complete signal
    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

    // Read results
    double *results;
    vkMapMemory(device, outMem, 0, BUFFER_SIZE, 0, (void **)&results);
    printf("out[0]=%f  out[1]=%f  out[1023]=%f\n",
           results[0], results[1], results[1023]);
    vkUnmapMemory(device, outMem);

    // 14. Cleanup (abbreviated)
    vkDestroyFence(device, fence, NULL);
    vkDestroyCommandPool(device, cmdPool, NULL);
    vkDestroyDescriptorPool(device, descPool, NULL);
    vkDestroyPipeline(device, pipeline, NULL);
    vkDestroyPipelineLayout(device, pipelineLayout, NULL);
    vkDestroyShaderModule(device, shaderMod, NULL);
    vkDestroyDescriptorSetLayout(device, descLayout, NULL);
    vkDestroyBuffer(device, stackBuf, NULL);  vkFreeMemory(device, stackMem, NULL);
    vkDestroyBuffer(device, constArgsBuf, NULL);  vkFreeMemory(device, constArgsMem, NULL);
    vkDestroyBuffer(device, outBuf, NULL); vkFreeMemory(device, outMem, NULL);
    vkDestroyDevice(device, NULL);
    vkDestroyInstance(instance, NULL);
    return 0;
}

JNIEXPORT void JNICALL
Java_com_endrf_LudicrousEngine_loadShader(JNIEnv* env, jclass cls, jlong instancePtr, jobject byteBuffer) {
    InstanceState* state = (InstanceState*)(intptr_t)instancePtr;
    if (!state) return;
    void* bufferPtr = (*env)->GetDirectBufferAddress(env, byteBuffer);
    if (!bufferPtr) return;
    uint32_t* spirvDataPtr = (uint32_t*)(bufferPtr);
    size_t len = (size_t)(*env)->GetDirectBufferCapacity(env, byteBuffer);
    //vulkan_test(spirvDataPtr, len);
    //log_info("test 1\n");
    //create_shader(state, spirvDataPtr, len);
}

JNIEXPORT jlong JNICALL
Java_com_endrf_LudicrousEngine_newInstance(JNIEnv* env, jclass cls) {
    InstanceState* state = create_instance(VK_QUEUE_COMPUTE_BIT);
    return (jlong)(intptr_t)state;
}

JNIEXPORT void JNICALL
Java_com_endrf_LudicrousEngine_setDensityFunction(JNIEnv* env, jclass cls, jlong instancePtr, jstring fnJSON) {
    InstanceState* state = (InstanceState*)(intptr_t)instancePtr;
    register_op(OP_ADD, 2, 1);
    register_op(OP_MUL, 3, 1);
    register_op(OP_MIN, 1, 1);
    register_op(OP_MAX, 1, 1);
    const char* json = (*env)->GetStringUTFChars(env, fnJSON, NULL);
    //printf("fd %s\n", json);

    yyjson_doc* doc = yyjson_read(json, strlen(json), 0);
    yyjson_val* json_root = yyjson_doc_get_root(doc);

    Node* root = build_tree_from_json_recursive(json_root);

    //root = tree_to_vine(root);
    EvalStack* stack = generate_stack_wrapper(root);
    state->data = (void*)stack;
    //double result = evaluate_stack(stack, 0, 0, 0);
    //printf("registers: %d\n", stack->max_reg+1);
    //printf("result: %.30lf\n", result);
    yyjson_doc_free(doc);
    fflush(stdout);
}

JNIEXPORT void JNICALL
Java_com_endrf_LudicrousEngine_setNoiseStates(JNIEnv* env, jclass cls, jlong instancePtr, jobject jNoiseStates, jobject jNoiseLevels) {
    InstanceState* state = (InstanceState*)(intptr_t)instancePtr;
    jclass listClass = (*env)->GetObjectClass(env, jNoiseStates);
    jmethodID sizeMethod = (*env)->GetMethodID(env, listClass, "size", "()I");
    jmethodID getMethod = (*env)->GetMethodID(env, listClass, "get", "(I)Ljava/lang/Object;");

    jint noiseStatesSize = (*env)->CallIntMethod(env, jNoiseStates, sizeMethod);
    jint noiseLevelsSize = (*env)->CallIntMethod(env, jNoiseLevels, sizeMethod);

    NormalNoise* noiseStates = (NormalNoise*)malloc(noiseStatesSize * sizeof(NormalNoise));
    jclass perlinNoiseCls = (*env)->FindClass(env, "com/endrf/NoiseStates$PerlinNoiseState");
    jfieldID perlinNoiseLowestFreqValFactorField = (*env)->GetFieldID(env, perlinNoiseCls, "lowestFreqValueFactor", "D");
    jfieldID perlinNoiseLowestFreqInputFactorField = (*env)->GetFieldID(env, perlinNoiseCls, "lowestFreqInputFactor", "D");
    jfieldID perlinNoiseLevelsCountField = (*env)->GetFieldID(env, perlinNoiseCls, "levelsCount", "I");
    jfieldID perlinNoiseLevelsIndexField = (*env)->GetFieldID(env, perlinNoiseCls, "levelsIndex", "I");
    jclass normalNoiseCls = (*env)->FindClass(env, "com/endrf/NoiseStates$NormalNoiseState");
    jfieldID normalNoiseNoisesField = (*env)->GetFieldID(env, normalNoiseCls, "noises", "[Lcom/endrf/NoiseStates$PerlinNoiseState;");
    jfieldID normalNoiseMaxValField = (*env)->GetFieldID(env, normalNoiseCls, "maxValue", "D");
    jfieldID normalNoiseValFactorField = (*env)->GetFieldID(env, normalNoiseCls, "valueFactor", "D");
    for (int i = 0; i < noiseStatesSize; i++) {
        jobject jNoiseObj = (*env)->CallObjectMethod(env, jNoiseStates, getMethod, i);
        jobjectArray jNoisesArr = (*env)->GetObjectField(env, jNoiseObj, normalNoiseNoisesField);
        for (int j = 0; j < 2; j++) {
            jobject jPerlinNoise = (*env)->GetObjectArrayElement(env, jNoisesArr, j);
            PerlinNoise perlinNoise = {
                .lowest_freq_value_factor = (*env)->GetDoubleField(env, jPerlinNoise, perlinNoiseLowestFreqValFactorField),
                .lowest_freq_input_factor = (*env)->GetDoubleField(env, jPerlinNoise, perlinNoiseLowestFreqInputFactorField),
                .levels_count = (*env)->GetIntField(env, jPerlinNoise, perlinNoiseLevelsCountField),
                .levels_index = (*env)->GetIntField(env, jPerlinNoise, perlinNoiseLevelsIndexField)
            };
            noiseStates[i].noises[j] = perlinNoise;
        }
        noiseStates[i].max_value = (*env)->GetDoubleField(env, jNoiseObj, normalNoiseMaxValField);
        noiseStates[i].value_factor = (*env)->GetDoubleField(env, jNoiseObj, normalNoiseValFactorField);
    }
    ImprovedNoise* noiseLevels = (ImprovedNoise*)malloc(noiseLevelsSize * sizeof(ImprovedNoise));
    jclass improvedNoiseCls = (*env)->FindClass(env, "com/endrf/NoiseStates$ImprovedNoiseState");
    jfieldID improvedNoiseXField = (*env)->GetFieldID(env, improvedNoiseCls, "x", "D");
    jfieldID improvedNoiseYField = (*env)->GetFieldID(env, improvedNoiseCls, "y", "D");
    jfieldID improvedNoiseZField = (*env)->GetFieldID(env, improvedNoiseCls, "z", "D");
    jfieldID improvedNoiseValuesField = (*env)->GetFieldID(env, improvedNoiseCls, "values", "[B");
    jfieldID improvedNoiseAmplitudeField = (*env)->GetFieldID(env, improvedNoiseCls, "amplitude", "D");
    jfieldID improvedNoiseDisabledField = (*env)->GetFieldID(env, improvedNoiseCls, "disabled", "Z");
    for (int i = 0; i < noiseLevelsSize; i++) {
        jobject jNoiseObj = (*env)->CallObjectMethod(env, jNoiseLevels, getMethod, i);
        uint8_t values[256];
        int8_t disabled = (*env)->GetBooleanField(env, jNoiseObj, improvedNoiseDisabledField);
        if (disabled) {
            memset(values, 0, sizeof(values));
        } else {
            jbyteArray valArr = (*env)->GetObjectField(env, jNoiseObj, improvedNoiseValuesField);
            int8_t byteValues[256];
            (*env)->GetByteArrayRegion(env, valArr, 0, 256, byteValues);
            for (int i = 0; i < 256; i++)
                values[i] = byteValues[i] & 0xFF;
        }
        noiseLevels[i] = (ImprovedNoise){
            .x = (*env)->GetDoubleField(env, jNoiseObj, improvedNoiseXField),
            .y = (*env)->GetDoubleField(env, jNoiseObj, improvedNoiseYField),
            .z = (*env)->GetDoubleField(env, jNoiseObj, improvedNoiseZField),
            .amplitude = (*env)->GetDoubleField(env, jNoiseObj, improvedNoiseAmplitudeField),
            .disabled = disabled
        };
        memcpy(noiseLevels[i].values, values, 256);
        memcpy(noiseLevels[i].values + 256, values, 256);
    }
    EvalStack* stack = (EvalStack*)state->data;
    stack->noise_states = noiseStates;
    stack->noise_levels = noiseLevels;
    struct timespec start, end;
    double result;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    result = generate_normal_noise(&noiseStates[12], noiseLevels, 57392152914566.49321, 67432010703250.432, -97419932495.33332);
    //result = generate_normal_noise(&noiseStates[39], noiseLevels, 0.0, 0.0, 0.0);
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    printf("normalnoise c: %.17lf\n", result);
    printf("time: %lld nanoseconds\n", (end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec));
    fflush(stdout);
}

JNIEXPORT jdouble JNICALL
Java_com_endrf_LudicrousEngine_evalDensityFunction(JNIEnv* env, jclass cls, jlong instancePtr, jint x, jint y, jint z) {
    InstanceState* state = (InstanceState*)(intptr_t)instancePtr;
    //struct timespec start, end;
    //clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    double result = evaluate_stack((EvalStack*)state->data, x, y, z);
    //clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    //printf("registers: %d\n", ((EvalStack*)state->data)->max_reg+1);
    //printf("result: %.30lf\n", result);
    //printf("time: %lld nanoseconds\n", (end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec));
    //fflush(stdout);
    return result;
}