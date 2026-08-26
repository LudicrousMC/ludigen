#include <vulkan/vulkan.h>
#include <jni.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>
#include "ludicrous_engine.h"
#include "op_stack.h"
#include "op_types.h"
#include "vulkan_util.h"
#include "shader.h"
#include "logger.h"
#include "yyjson.h"
#include "stack_builder.h"
#include "noise_gen.h"
#include "util.h"

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
    sem_init(&state->fd_sem, 0, 1);
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
        op_type == OP_SQUEEZE ||
        op_type == OP_INTERPOLATED )
    {
        Node* arg1 = build_tree_from_json_recursive(yyjson_obj_get(json, "argument"));
        return create_op(op_type, &arg1, NULL);
    }
    if (op_type == OP_CACHE_ONCE ||
        op_type == OP_FLAT_CACHE ||
        op_type == OP_CACHE_2D)
    {
        Node* arg1 = build_tree_from_json_recursive(yyjson_obj_get(json, "argument"));
        int index = yyjson_get_int(yyjson_obj_get(json, "index"));
        return create_op(op_type, &arg1, (double[]){(double)index});
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
    double result = evaluate_stack(stack, 0, 0, 0, NULL);
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
          .flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          .sharing = VK_SHARING_MODE_EXCLUSIVE },
        { .buf = NULL, .size = sizeof(double) * 1024,
          .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
          .flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, .sharing = VK_SHARING_MODE_EXCLUSIVE },
        { .buf = NULL, .size = sizeof(double) * 1024, .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
          .flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          .sharing = VK_SHARING_MODE_EXCLUSIVE }
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
    EvalStack* stack = state->final_density;
    /* StackFrame frames[1] = {0};
    frames[0] = (StackFrame){
        .op_type = OP_ADD,
        .arg_indexes = {-1, -2, 0},
        .const_index = 0,
        .result_index = 0
    };
    double const_args[2] = {0};
    const_args[0] = 6.0;
    const_args[1] = 7.0; */
    ShaderBufferData buffers[] = {
        // CPU Staging buffer for frames
        { .buf = stack->op_stack, .size = sizeof(StackFrame) * stack->op_stack_len,
          .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
          .flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          .sharing = VK_SHARING_MODE_EXCLUSIVE },
        // GPU Storage buffer for frames
        { .buf = NULL, .size = sizeof(StackFrame) * stack->op_stack_len,
          .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
          .flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, .sharing = VK_SHARING_MODE_EXCLUSIVE },
        // CPU Staging buffer for const_args
        { .buf = stack->const_args, .size = sizeof(double) * stack->const_args_len,
          .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
          .flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          .sharing = VK_SHARING_MODE_EXCLUSIVE },
        // GPU Storage buffer for const_args
        { .buf = NULL, .size = sizeof(double) * stack->const_args_len,
          .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
          .flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, .sharing = VK_SHARING_MODE_EXCLUSIVE },
        // CPU Staging buffer for noise_states
        { .buf = stack->noise_states, .size = sizeof(NormalNoise) * stack->noise_states_len,
          .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
          .flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          .sharing = VK_SHARING_MODE_EXCLUSIVE },
        // GPU Storage buffer for noise_states
        { .buf = NULL, .size = sizeof(NormalNoise) * stack->noise_states_len,
          .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
          .flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, .sharing = VK_SHARING_MODE_EXCLUSIVE },
        // CPU Staging buffer for noise_levels
        { .buf = stack->noise_levels, .size = sizeof(ImprovedNoise) * stack->noise_levels_len,
          .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
          .flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          .sharing = VK_SHARING_MODE_EXCLUSIVE },
        // GPU Storage buffer for noise_levels
        { .buf = NULL, .size = sizeof(ImprovedNoise) * stack->noise_levels_len,
          .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
          .flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, .sharing = VK_SHARING_MODE_EXCLUSIVE },
        // GPU Storage buffer for output
        { .buf = NULL, .size = sizeof(double) * 98304,
          .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
          .flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, .sharing = VK_SHARING_MODE_EXCLUSIVE },
        // CPU Staging buffer for output
        { .buf = NULL, .size = sizeof(double) * 98304,
          .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
          .flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          .sharing = VK_SHARING_MODE_EXCLUSIVE }
    };
    VkDescriptorSetLayoutBinding desc_set_layout[] = {
        { .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
        { .binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
        { .binding = 2, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
        { .binding = 3, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
        { .binding = 4, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT }
    };
    ShaderDescriptorLayout desc_layouts[] = {
        { .bindings = desc_set_layout,
          .num_bindings = sizeof(desc_set_layout) / sizeof(desc_set_layout[0]) }
    };
    ShaderDescriptorSetUpdate desc_updates[] = {
        { .desc_index = 0, .mem_indexes = (size_t[]){1},
          .buf_indexes = (size_t[]){0}, .desc_count = 1, .set_index = 0 },
        { .desc_index = 1, .mem_indexes = (size_t[]){3},
          .buf_indexes = (size_t[]){0}, .desc_count = 1, .set_index = 0 },
        { .desc_index = 2, .mem_indexes = (size_t[]){5},
          .buf_indexes = (size_t[]){0}, .desc_count = 1, .set_index = 0 },
        { .desc_index = 3, .mem_indexes = (size_t[]){7},
          .buf_indexes = (size_t[]){0}, .desc_count = 1, .set_index = 0 },
        { .desc_index = 4, .mem_indexes = (size_t[]){8},
          .buf_indexes = (size_t[]){0}, .desc_count = 1, .set_index = 0 }
    };
    VkSpecializationInfo spec = {
        .mapEntryCount = 1,
        .pMapEntries = &(VkSpecializationMapEntry){
            .constantID = 0, .offset = 0, .size = sizeof(uint32_t) },
        .dataSize = sizeof(uint32_t),
        .pData = &(int){stack->max_reg + 1}
    };
    ShaderCreateData create_data = {
        .buffers = buffers,
        .num_buffers = sizeof(buffers) / sizeof(buffers[0]),
        .desc_layouts = desc_layouts,
        .num_desc_layouts = sizeof(desc_layouts) / sizeof(desc_layouts[0]),
        .desc_updates = desc_updates,
        .num_desc_updates = sizeof(desc_updates) / sizeof(desc_updates[0]),
        .spec = &spec
    };
    ShaderData* shader = (ShaderData*)malloc(sizeof(ShaderData));
    create_shader(state, spirvDataPtr, len, &create_data, shader);

    vkBeginCommandBuffer(shader->vk_cmd, &(VkCommandBufferBeginInfo){
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
    });
    // Copy frames staging buffer to GPU
    vkCmdCopyBuffer(
        shader->vk_cmd,
        shader->shader_mem[0].vk_buf[0],
        shader->shader_mem[1].vk_buf[0],
        1,
        &(VkBufferCopy){ .size = sizeof(StackFrame) * stack->op_stack_len }
    );
    // Copy const_args staging buffer to GPU
    vkCmdCopyBuffer(
        shader->vk_cmd,
        shader->shader_mem[2].vk_buf[0],
        shader->shader_mem[3].vk_buf[0],
        1,
        &(VkBufferCopy){ .size = sizeof(double) * stack->const_args_len }
    );
    // Copy noise_levels staging buffer to GPU
    vkCmdCopyBuffer(
        shader->vk_cmd,
        shader->shader_mem[4].vk_buf[0],
        shader->shader_mem[5].vk_buf[0],
        1,
        &(VkBufferCopy){ .size = sizeof(NormalNoise) * stack->noise_states_len }
    );
    // Copy noise_levels staging buffer to GPU
    vkCmdCopyBuffer(
        shader->vk_cmd,
        shader->shader_mem[6].vk_buf[0],
        shader->shader_mem[7].vk_buf[0],
        1,
        &(VkBufferCopy){ .size = sizeof(ImprovedNoise) * stack->noise_levels_len }
    );
    vkCmdBindPipeline(shader->vk_cmd, VK_PIPELINE_BIND_POINT_COMPUTE, shader->vk_pipeline);
    vkCmdBindDescriptorSets(
        shader->vk_cmd,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        shader->vk_pipeline_layout,
        0,
        shader->vk_desc_sets_len,
        shader->vk_desc_sets,
        0,
        NULL
    );
    VkBufferMemoryBarrier barriers[4] = {
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = shader->shader_mem[1].vk_buf[0],
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        },
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = shader->shader_mem[3].vk_buf[0],
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        },
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = shader->shader_mem[5].vk_buf[0],
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        },
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = shader->shader_mem[7].vk_buf[0],
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        },
    };

    vkCmdPipelineBarrier(
        shader->vk_cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0,
        0, NULL,
        4, barriers,
        0, NULL
    );
    vkCmdPushConstants(
        shader->vk_cmd,
        shader->vk_pipeline_layout,
        VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        sizeof(int) * 3,
        &(int[]){0, 0, 0}
    );
    vkCmdDispatch(shader->vk_cmd, 98304 / 64, 1, 1);
    VkBufferMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = shader->shader_mem[8].vk_buf[0],
        .offset = 0,
        .size = VK_WHOLE_SIZE
    };
    vkCmdPipelineBarrier(
        shader->vk_cmd,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, NULL,
        1, &barrier,
        0, NULL
    );
    // Copy GPU output buffer to CPU
    vkCmdCopyBuffer(
        shader->vk_cmd,
        shader->shader_mem[8].vk_buf[0],
        shader->shader_mem[9].vk_buf[0],
        1,
        &(VkBufferCopy){ .srcOffset = 0, .dstOffset = 0, .size = sizeof(double) * 98304 }
    );
    vkEndCommandBuffer(shader->vk_cmd);

    VkFence fence;
    vkCreateFence(state->vk_device,
        &(VkFenceCreateInfo){
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
        },
        NULL, &fence
    );
    int* tmp = realloc(state->shaders, state->shaders_len + 1);
    if (tmp != NULL) {
        state->shaders[state->shaders_len] = shader;
        state->shaders_len++;
    }
    state->fd_fence = fence;
    vkQueueSubmit(state->vk_queue, 1,
        &(VkSubmitInfo){
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &(shader->vk_cmd)
        },
        fence
    );
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);

    vkWaitForFences(state->vk_device, 1, &fence, VK_TRUE, UINT64_MAX);

    double* results;
    vkMapMemory(
        state->vk_device,
        shader->shader_mem[9].vk_mem,
        0,
        sizeof(double) * 98304,
        0,
        (void**)&results
    );
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);

    printf("GPU out[0]=%lf  out[1]=%lf  out[98303]=%lf\n",
           results[0], results[1], results[98303]);
    printf("time: %lld nanoseconds\n", (end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec));
    fflush(stdout);
    vkUnmapMemory(state->vk_device, shader->shader_mem[9].vk_mem);
    double* test_arr = (double*)malloc(sizeof(double) * 98304);

    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    for (int i = 0; i < 98304; i++) {
        test_arr[i] = evaluate_stack(stack, i, 70, 0, NULL);
    }
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    printf("CPU out[0]=%lf  out[1]=%lf  out[98303]=%lf\n",
               test_arr[0], test_arr[1], test_arr[98303]);
    printf("time: %lld nanoseconds\n", (end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec));
    fflush(stdout);
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
    state->final_density = stack;
    //double result = evaluate_stack(stack, 0, 0, 0);
    printf("registers: %d\n", stack->max_reg+1);
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
    EvalStack* stack = state->final_density;
    stack->noise_states = noiseStates;
    stack->noise_states_len = noiseStatesSize;
    stack->noise_levels = noiseLevels;
    stack->noise_levels_len = noiseLevelsSize;

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

JNIEXPORT void JNICALL
Java_com_endrf_LudicrousEngine_fillNoiseAccel(JNIEnv* env, jclass cls, jlong instancePtr, jint min_y, jint height, jint cell_width, jint cell_height, jint x, jint z, jdoubleArray j_densities) {
    InstanceState* state = (InstanceState*)(intptr_t)instancePtr;
    int size = height * 16 * 16;
    sem_wait(&state->fd_sem);
    vkResetCommandBuffer(state->shaders[0]->vk_cmd, 0);
    vkResetFences(state->vk_device, 1, &state->fd_fence);
    vkBeginCommandBuffer(state->shaders[0]->vk_cmd, &(VkCommandBufferBeginInfo){
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
    });
    vkCmdBindPipeline(state->shaders[0]->vk_cmd, VK_PIPELINE_BIND_POINT_COMPUTE, state->shaders[0]->vk_pipeline);
    vkCmdBindDescriptorSets(
        state->shaders[0]->vk_cmd,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        state->shaders[0]->vk_pipeline_layout,
        0,
        state->shaders[0]->vk_desc_sets_len,
        state->shaders[0]->vk_desc_sets,
        0,
        NULL
    );
    vkCmdPushConstants(
        state->shaders[0]->vk_cmd,
        state->shaders[0]->vk_pipeline_layout,
        VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        sizeof(int) * 3,
        &(int[]){x, min_y, z}
    );
    vkCmdDispatch(state->shaders[0]->vk_cmd, size / 64, 1, 1);
    vkCmdPipelineBarrier(
        state->shaders[0]->vk_cmd,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, NULL, 1,
        &(VkBufferMemoryBarrier){
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = state->shaders[0]->shader_mem[8].vk_buf[0],
            .offset = 0,
            .size = VK_WHOLE_SIZE
        },
        0, NULL
    );
    // Copy GPU output buffer to CPU
    vkCmdCopyBuffer(
        state->shaders[0]->vk_cmd,
        state->shaders[0]->shader_mem[8].vk_buf[0],
        state->shaders[0]->shader_mem[9].vk_buf[0],
        1,
        &(VkBufferCopy){ .srcOffset = 0, .dstOffset = 0, .size = sizeof(double) * size }
    );
    vkEndCommandBuffer(state->shaders[0]->vk_cmd);
    vkQueueSubmit(state->vk_queue, 1,
        &(VkSubmitInfo){
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &(state->shaders[0]->vk_cmd)
        },
        state->fd_fence
    );
    vkWaitForFences(state->vk_device, 1, &state->fd_fence, VK_TRUE, UINT64_MAX);
    double* densities;
    vkMapMemory(
        state->vk_device,
        state->shaders[0]->shader_mem[9].vk_mem,
        0,
        sizeof(double) * size,
        0,
        (void**)&densities
    );
    /* jdoubleArray j_densities = (*env)->NewDoubleArray(env, size);
    if (j_densities == NULL) {
        return NULL;
    }*/
    jdouble* dst = (*env)->GetPrimitiveArrayCritical(env, j_densities, NULL);
    if (dst == NULL) {
        return;
    }
    memcpy(dst, densities, size * sizeof(jdouble));
    (*env)->ReleasePrimitiveArrayCritical(env, j_densities, dst, 0);
    //(*env)->SetDoubleArrayRegion(env, j_densities, 0, size, (jdouble*)densities);
    vkUnmapMemory(state->vk_device, state->shaders[0]->shader_mem[0].vk_mem);
    sem_post(&state->fd_sem);
    //free(densities);
}

JNIEXPORT jdoubleArray JNICALL
Java_com_endrf_LudicrousEngine_fillNoise(JNIEnv* env, jclass cls, jlong instancePtr, jint min_y, jint height, jint cell_width, jint cell_height, jint x, jint z) {
    InstanceState* state = (InstanceState*)(intptr_t)instancePtr;
    //struct timespec start, end;
    //clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    int max_y = height + min_y;
    int xz_samples = (16 / cell_width) + 1;


    int size = height * 16 * 16;
    double* densities = (double*)malloc(size * sizeof(double));
    int num_interp_functions = state->final_density->interp_len;
    int num_cache_once_functions = state->final_density->cache_once_len;
    int num_cache_flat_functions = state->final_density->cache_flat_len;
    int num_cache_2d_functions = state->final_density->cache_2d_len;

    EvalCache eval_cache;
    if (num_interp_functions == 0) {
        eval_cache = (EvalCache){0};
    } else {
        double* nan_template = (double*)malloc(num_interp_functions * sizeof(double));
        fill_nan(nan_template, num_interp_functions);
        int interp_size = xz_samples * xz_samples * num_interp_functions;
        double* interp = (double*)malloc(interp_size * sizeof(double));
        fill_nan(interp, interp_size);
        double* active_cell_top = (double*)malloc(4 * num_interp_functions * sizeof(double));
        fill_nan(active_cell_top, 4 * num_interp_functions);
        int cache_once_size = (height + cell_height + 1) * 17 * 17 * num_cache_once_functions;
        double* cache_once = (double*)malloc(cache_once_size * sizeof(double));
        fill_nan(cache_once, cache_once_size);
        int cache_flat_size = xz_samples * xz_samples * num_cache_flat_functions;
        double* cache_flat = (double*)malloc(cache_flat_size * sizeof(double));
        fill_nan(cache_flat, cache_flat_size);
        int cache_2d_size = 17 * 17 * num_cache_2d_functions;
        double* cache_2d = (double*)malloc(cache_2d_size * sizeof(double));
        fill_nan(cache_2d, cache_2d_size);


        eval_cache = (EvalCache){
            .interp_enabled = 1,
            .cache_once_enabled = 1,
            .cache_flat_enabled = 1,
            .cache_2d_enabled = 1,
            .cell_width = cell_width,
            .cell_height = cell_height,
            .xz_samples = xz_samples,
            .min_y = min_y,
            .num_interp_functions = num_interp_functions,
            .nan_template = nan_template,
            .interp = interp,
            .active_cell_top = active_cell_top,
            .cell_x = 0,
            .cell_z = 0,
            .num_cache_once_functions = num_cache_once_functions,
            .cache_once = cache_once,
            .num_cache_flat_functions = num_cache_flat_functions,
            .cache_flat = cache_flat,
            .num_cache_2d_functions = num_cache_2d_functions,
            .cache_2d = cache_2d
        };
    }

    /* int y_samples = (height / cell_height) + 2;
    int sample_size = y_samples * xz_samples * xz_samples;
    double* samples = (double*)malloc(sample_size * sizeof(double));
    for (int y_in_chunk = min_y, y_index = 0; y_in_chunk <= max_y + 1; y_in_chunk += cell_height, y_index++) {
        for (int z_in_chunk = 0, z_index = 0; z_in_chunk <= 16; z_in_chunk += cell_width, z_index++) {
            for (int x_in_chunk = 0, x_index = 0; x_in_chunk <= 16; x_in_chunk += cell_width, x_index++) {
                samples[(y_index * xz_samples * xz_samples) + (z_index * xz_samples) + x_index] = evaluate_stack((EvalStack*)state->data, x + x_in_chunk, y_in_chunk, z + z_in_chunk, &eval_cache);
                //reset_cache_once(&eval_cache);
                eval_cache.cell_x++;
            }
            eval_cache.cell_x = 0;
            eval_cache.cell_z++;
        }
        eval_cache.cell_x = 0;
        eval_cache.cell_z = 0;
    } */

    int num_y_cells = height / cell_height;
    int num_z_cells = 16 / cell_width;
    int num_x_cells = 16 / cell_width;
    for (int cell_y = 0; cell_y < num_y_cells; cell_y++) {
        for (int cell_z = 0; cell_z < num_z_cells; cell_z++) {
            for (int cell_x = 0; cell_x < num_x_cells; cell_x++) {
                for (int block_y = 0; block_y < cell_height; block_y++) {
                    int y_in_chunk = cell_y * cell_height + block_y;
                    if (y_in_chunk + min_y == max_y)
                        goto end_loop;
                    for (int block_z = 0; block_z < cell_width; block_z++) {
                        int z_in_chunk = cell_z * cell_width + block_z;
                        for (int block_x = 0; block_x < cell_width; block_x++) {
                            int x_in_chunk = cell_x * cell_width + block_x;
                            densities[
                                y_in_chunk * 256 +
                                z_in_chunk * 16 +
                                x_in_chunk
                            ] = evaluate_stack(state->final_density, x + x_in_chunk, y_in_chunk + min_y, z + z_in_chunk, &eval_cache);
                        }
                    }
                }
                if (cell_x != num_x_cells - 1)
                    advance_cell_x(&eval_cache);
            }
            if (cell_z != num_z_cells - 1) {
                advance_cell_z(&eval_cache);
            }
        }
        if (cell_y != num_y_cells - 1) {
            advance_cell_y(&eval_cache);
        }
    }
    end_loop:

    /* for (int y_in_chunk = min_y, y_index = 0; y_in_chunk <= max_y; y_in_chunk++, y_index++) {
        for (int z_in_chunk = 0; z_in_chunk < 16; z_in_chunk++) {
            for (int x_in_chunk = 0; x_in_chunk < 16; x_in_chunk++) {
                if (y_index % 8 == 0 && z_in_chunk % 4 == 0 && x_in_chunk % 4 == 0) {
                    int density_index = (y_index * 256) + (z_in_chunk * 16) + x_in_chunk;
                    int sample_index = (y_index * xz_samples * xz_samples / 8) + (z_in_chunk * xz_samples / 4) + (x_in_chunk / 4);
                    densities[density_index] = samples[sample_index];
                    continue;
                }

                int sy = y_index / cell_height;
                int sz = z_in_chunk / cell_width;
                int sample_x = x_in_chunk / cell_width;
                int sample_y = sy * xz_samples * xz_samples;
                int sample_z = sz * xz_samples;
                int sample_x1 = sample_x + 1;
                int sample_y1 = (sy + 1) * xz_samples * xz_samples;
                int sample_z1 = (sz + 1) * xz_samples;

                int i000 = sample_x + sample_y + sample_z;
                int i100 = sample_x1 + sample_y + sample_z;
                int i001 = sample_x + sample_y + sample_z1;
                int i101 = sample_x1 + sample_y + sample_z1;
                int i010 = sample_x + sample_y1 + sample_z;
                int i110 = sample_x1 + sample_y1 + sample_z;
                int i011 = sample_x + sample_y1 + sample_z1;
                int i111 = sample_x1 + sample_y1 + sample_z1;

                double cell_pos_x = (double)x_in_chunk / cell_width;
                double cell_pos_y = (double)y_index / cell_height;
                double cell_pos_z = (double)z_in_chunk / cell_width;
                double pos_x = cell_pos_x - (int)cell_pos_x;
                double pos_y = cell_pos_y - (int)cell_pos_y;
                double pos_z = cell_pos_z - (int)cell_pos_z;

                double x00 = lerp(pos_x, samples[i000], samples[i100]);
                double x01 = lerp(pos_x, samples[i001], samples[i101]);
                double x10 = lerp(pos_x, samples[i010], samples[i110]);
                double x11 = lerp(pos_x, samples[i011], samples[i111]);

                double z0 = lerp(pos_z, x00, x01);
                double z1 = lerp(pos_z, x10, x11);

                int density_index = (y_index * 256) + (z_in_chunk * 16) + x_in_chunk;
                densities[density_index] = lerp(pos_y, z0, z1);
            }
        }
    } */

    jdoubleArray j_densities = (*env)->NewDoubleArray(env, size);
    if (j_densities == NULL) {
        free(densities);
        return NULL;
    }
    jdouble* dst = (*env)->GetPrimitiveArrayCritical(env, j_densities, NULL);
    if (dst == NULL) {
        return NULL;
    }
    memcpy(dst, densities, size * sizeof(jdouble));
    (*env)->ReleasePrimitiveArrayCritical(env, j_densities, dst, 0);
    //(*env)->SetDoubleArrayRegion(env, j_densities, 0, size, (jdouble*)densities);
    free(densities);
    //free(samples);
    free(eval_cache.nan_template);
    free(eval_cache.interp);
    free(eval_cache.active_cell_top);
    free(eval_cache.cache_once);
    free(eval_cache.cache_flat);
    free(eval_cache.cache_2d);
    //clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    //printf("time: %lld nanoseconds\n", (end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec));
    //fflush(stdout);
    return j_densities;
}

JNIEXPORT jdouble JNICALL
Java_com_endrf_LudicrousEngine_evalDensityFunction(JNIEnv* env, jclass cls, jlong instancePtr, jint x, jint y, jint z) {
    InstanceState* state = (InstanceState*)(intptr_t)instancePtr;
    //struct timespec start, end;
    //clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    double result = evaluate_stack(state->final_density, x, y, z, NULL);
    //printf("num interps %d\n", ((EvalStack*)state->data)->cache_once_len);
    //fflush(stdout);
    //clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    //printf("registers: %d\n", ((EvalStack*)state->data)->max_reg+1);
    //printf("result: %.30lf\n", result);
    //printf("time: %lld nanoseconds\n", (end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec));
    //fflush(stdout);
    return result;
}