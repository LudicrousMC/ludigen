#include <vulkan/vulkan.h>
#include <jni.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "tinycthread.h"
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
#include "eval.comp.h"

#define VULKAN_VERSION VK_API_VERSION_1_1

static JavaVM* java_vm = NULL;
JavaVM* get_java_vm() {
    return java_vm;
}

// Usage: InstanceState* state = create_instance(VK_QUEUE_COMPUTE_BIT);
static InstanceState* create_instance(int queueFamilyType) {

    // <-- Setup Vulkan Instance -->
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
    VkPhysicalDevice* physDevHdl = malloc(sizeof(VkPhysicalDevice) * 2);
    err = vkEnumeratePhysicalDevices(instance, &(uint32_t){2}, physDevHdl);
    if (err < VK_SUCCESS)
        return NULL;

    VkPhysicalDevice physDev = physDevHdl[0];
    VkPhysicalDeviceIDProperties physDevIDProps = {0};
    physDevIDProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
    physDevIDProps.pNext = NULL;
    VkPhysicalDeviceProperties2 physDevProps2 = {0};
    physDevProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    physDevProps2.pNext = &physDevIDProps;
    char physDevUUID[VK_UUID_SIZE * 2 + 5]= {0};
    vkGetPhysicalDeviceProperties2(physDev, &physDevProps2);
    for (int i = 0, s = 0, uuid_pos = 0; i < VK_UUID_SIZE; i++, s += 2) {
        if (i == 4 || i == 6 || i == 8 || i == 10) {
            physDevUUID[s] = '-';
            s++;
        }
        uuid_pos += snprintf(&physDevUUID[s], sizeof(physDevUUID) - uuid_pos, "%02x", physDevIDProps.deviceUUID[i]);

    }
    log_info("Selected \"%s\" (ID=%s) as Primary GPU Device", physDevProps2.properties.deviceName, physDevUUID);
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
    if (err != VK_SUCCESS) {
        log_err("Error creating vulkan device. VK Code: %d", err);
        return NULL;
    }

    // <-- Get Logical Device's Created Queue -->
    VkQueue queue;
    vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);

    InstanceState* state = (InstanceState*)malloc(sizeof(InstanceState));
    if (state == NULL) {
        log_err("Error during malloc for instance state");
        return NULL;
    }
    int cache_map_size = 1 << 12; // 4096
    int max_alloc_bytes = (sizeof(ChunkData) + sizeof(float) * 384 * 16 * 16) * cache_map_size;
    log_info("Initializing chunk cache with capacity of %d chunks (max allocation: %d bytes)", cache_map_size, max_alloc_bytes);
    CacheMap cache_map = { .chunks = calloc(cache_map_size, sizeof(ChunkData)) };
    for (int i = 0; i < cache_map_size; i++) {
        mtx_init(&cache_map.chunks[i].sem, mtx_plain);
        cache_map.chunks[i].densities = NULL;
    }
    *state = (InstanceState){
        .vk_instance = instance,
        .vk_phys_device = physDev,
        .vk_queue_family_index = queueFamilyIndex,
        .vk_device = device,
        .vk_queue = queue,
        .shaders = NULL,
        .final_density = NULL,
        .shaders_len = 0,
        .fd_sem = {0},
        .cache_map = cache_map,
        .cache_map_len = cache_map_size
    };
    mtx_init(&state->fd_sem, mtx_plain);
    return state;
}

uint64_t pack_chunk_identifier(int x, int z, int fd_index) {
    return ((uint64_t)(x / 16) << 36) | ((uint64_t)(z / 16) << 8) | (fd_index & 0xFF);
}

uint64_t splitmix64(uint64_t value) {
    value ^= value >> 30;
    value *= 0xbf58476d1ce4e5b9ULL;
    value ^= value >> 27;
    value *= 0x94d049bb133111ebULL;
    value ^= value >> 31;
    return value;
}

uint64_t get_cached_chunk_index(size_t len, int x, int z, int fd_index) {
    return splitmix64(pack_chunk_identifier(x, z, fd_index)) & (len - 1);
}

bool cache_map_contains(CacheMap* cache_map, uint64_t index, int x, int z, int fd_index) {
    mtx_lock(&cache_map->chunks[index].sem);
    if (cache_map->chunks[index].x != x ||
        cache_map->chunks[index].z != z ||
        cache_map->chunks[index].fd_index != fd_index ||
        cache_map->chunks[index].densities == NULL
    ) {
        mtx_unlock(&cache_map->chunks[index].sem);
        return false;
    }
    mtx_unlock(&cache_map->chunks[index].sem);
    return true;
}

float* cache_map_get(CacheMap* cache_map, uint64_t index, int x, int z, int fd_index) {
    mtx_lock(&cache_map->chunks[index].sem);
    if (cache_map->chunks[index].x != x ||
        cache_map->chunks[index].z != z ||
        cache_map->chunks[index].fd_index != fd_index ||
        cache_map->chunks[index].densities == NULL
    ) {
        mtx_unlock(&cache_map->chunks[index].sem);
        return NULL;
    }
    return cache_map->chunks[index].densities;
}

bool cache_map_release(CacheMap* cache_map, uint64_t index) {
    free(cache_map->chunks[index].densities);
    cache_map->chunks[index].densities = NULL;
    mtx_unlock(&cache_map->chunks[index].sem);
    return true;
}

void cache_map_put(CacheMap* cache_map, size_t len, int x, int z, int fd_index, float* densities) {
    uint64_t index = get_cached_chunk_index(len, x, z, fd_index);
    mtx_lock(&cache_map->chunks[index].sem);
    if (cache_map->chunks[index].densities != NULL) {
        free(cache_map->chunks[index].densities);
    }
    cache_map->chunks[index].x = x;
    cache_map->chunks[index].z = z;
    cache_map->chunks[index].fd_index = fd_index;
    cache_map->chunks[index].densities = densities;
    mtx_unlock(&cache_map->chunks[index].sem);
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
        op_type == OP_INTERPOLATED)
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

void set_noise_states(JNIEnv* env, EvalStack* stack, jobject jNoiseStates, jobject jNoiseLevels) {
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
    jfieldID improvedNoiseDisabledField = (*env)->GetFieldID(env, improvedNoiseCls, "disabled", "B");
    for (int i = 0; i < noiseLevelsSize; i++) {
        jobject jNoiseObj = (*env)->CallObjectMethod(env, jNoiseLevels, getMethod, i);
        uint8_t values[256];
        int8_t disabled = (*env)->GetByteField(env, jNoiseObj, improvedNoiseDisabledField);
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
    stack->noise_states = noiseStates;
    stack->noise_states_len = noiseStatesSize;
    stack->noise_levels = noiseLevels;
    stack->noise_levels_len = noiseLevelsSize;

    /* printf("states: %d\n", noiseStatesSize);
    printf("levels: %d\n", noiseLevelsSize);
    fflush(stdout); */
}

JNIEXPORT void JNICALL
Java_com_endrf_LudicrousEngine_loadFinalDensityShader(
    JNIEnv* env,
    jclass cls,
    jlong instancePtr,
    jint scale,
    jint minY,
    jint height,
    jint noiseX,
    jint noiseY,
    jstring fnJSON,
    jobject jNoiseStates,
    jobject jNoiseLevels,
    jstring dimension
) {
    const char* dim_str = (*env)->GetStringUTFChars(env, dimension, NULL);
    if (dim_str == NULL) {
        dim_str = "\0";
    }
    struct timespec start, end;
    clock_gettime(TIME_UTC, &start);

    InstanceState* state = (InstanceState*)(intptr_t)instancePtr;
    if (!state) return;

    // Compile Final Density Pipeline from JSON
    register_op(OP_ADD, 2, 1);
    register_op(OP_MUL, 3, 1);
    register_op(OP_MIN, 1, 1);
    register_op(OP_MAX, 1, 1);
    struct timespec jsont_start, jsont_end;
    clock_gettime(TIME_UTC, &jsont_start);
    const char* json = (*env)->GetStringUTFChars(env, fnJSON, NULL);

    yyjson_doc* doc = yyjson_read(json, strlen(json), 0);
    yyjson_val* json_root = yyjson_doc_get_root(doc);
    Node* root = build_tree_from_json_recursive(json_root);
    yyjson_doc_free(doc);

    EvalStack* stack = (EvalStack*)malloc(sizeof(EvalStack));
    set_noise_states(env, stack, jNoiseStates, jNoiseLevels);
    generate_stack_wrapper(root, stack);
    clock_gettime(TIME_UTC, &jsont_end);
    long long json_time = (jsont_end.tv_sec - jsont_start.tv_sec) * 1000LL + (jsont_end.tv_nsec - jsont_start.tv_nsec) / 1000000LL;
    log_debug("Max register usage for level \"%s\" final density function: %d slots", dim_str, stack->max_reg + 1);

    stack->scale = scale;
    stack->min_y = minY;
    stack->height = height;
    stack->cell_size_x = noiseX;
    stack->cell_size_y = noiseY;
    //stack->interp_len = stack->interp_len != 0 ? stack->interp_len : 1;
    stack->cache_once_len = stack->cache_once_len != 0 ? stack->cache_once_len : 1;
    stack->cache_flat_len = stack->cache_flat_len != 0 ? stack->cache_flat_len : 1;
    stack->cache_2d_len = stack->cache_2d_len != 0 ? stack->cache_2d_len : 1;

    // Build Final Density Compute Shader
    int width = 16 * scale;
    int cell_width = noiseX << 2;
    int cell_height = noiseY << 2;
    int xz_samples = (width / cell_width) + 1;
    int y_samples = (height / cell_height) + 1;
    int size = height * width * width;
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
        // CPU Staging buffer for interp_indexes
        { .buf = stack->interp_pc_indexes, .size = sizeof(uint32_t) * stack->interp_len,
          .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
          .flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          .sharing = VK_SHARING_MODE_EXCLUSIVE },
        // GPU Storage buffer for interp_indexes
        { .buf = NULL, .size = sizeof(uint32_t) * stack->interp_len,
          .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
          .flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, .sharing = VK_SHARING_MODE_EXCLUSIVE },
        // GPU Storage buffer for output
        { .buf = NULL, .size = sizeof(float) * size,
          .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
          .flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, .sharing = VK_SHARING_MODE_EXCLUSIVE },
        // CPU Staging buffer for output
        { .buf = NULL, .size = sizeof(float) * size,
          .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
          .flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
          .sharing = VK_SHARING_MODE_EXCLUSIVE },
        // GPU Storage buffer for interpolation global cache
        { .buf = NULL, .size = sizeof(float) * xz_samples * xz_samples * y_samples * stack->interp_len,
          .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, .flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
          .sharing = VK_SHARING_MODE_EXCLUSIVE },
        // GPU Storage buffer for cache flat global
        { .buf = NULL, .size = sizeof(float) * xz_samples * xz_samples * stack->cache_flat_len,
          .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, .flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
          .sharing = VK_SHARING_MODE_EXCLUSIVE },
        // GPU Storage buffer for cache 2d global
        { .buf = NULL, .size = sizeof(float) * (width+1) * (width+1) * stack->cache_2d_len,
          .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, .flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
          .sharing = VK_SHARING_MODE_EXCLUSIVE }
    };
    VkDescriptorSetLayoutBinding desc_set_layout1[] = {
        { .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
        { .binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
        { .binding = 2, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
        { .binding = 3, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
        { .binding = 4, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
        { .binding = 5, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
    };
    VkDescriptorSetLayoutBinding desc_set_layout2[] = {
        { .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
        { .binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
        { .binding = 2, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT },
    };
    ShaderDescriptorLayout desc_layouts[] = {
        { .bindings = desc_set_layout1,
          .num_bindings = sizeof(desc_set_layout1) / sizeof(desc_set_layout1[0]) },
        { .bindings = desc_set_layout2,
          .num_bindings = sizeof(desc_set_layout2) / sizeof(desc_set_layout2[0]) }
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
        { .desc_index = 4, .mem_indexes = (size_t[]){9},
          .buf_indexes = (size_t[]){0}, .desc_count = 1, .set_index = 0 },
        { .desc_index = 5, .mem_indexes = (size_t[]){10},
          .buf_indexes = (size_t[]){0}, .desc_count = 1, .set_index = 0 },
        { .desc_index = 0, .mem_indexes = (size_t[]){12},
          .buf_indexes = (size_t[]){0}, .desc_count = 1, .set_index = 1 },
        { .desc_index = 1, .mem_indexes = (size_t[]){13},
          .buf_indexes = (size_t[]){0}, .desc_count = 1, .set_index = 1 },
        { .desc_index = 2, .mem_indexes = (size_t[]){14},
          .buf_indexes = (size_t[]){0}, .desc_count = 1, .set_index = 1 }
    };
    VkSpecializationInfo spec = {
        .mapEntryCount = 9,
        .pMapEntries = (VkSpecializationMapEntry[]){
            { .constantID = 0, .offset = 0, .size = sizeof(uint32_t) }, // Num registers
            { .constantID = 1, .offset = sizeof(uint32_t), .size = sizeof(uint32_t) }, // Height
            { .constantID = 2, .offset = 2 *sizeof(uint32_t), .size = sizeof(uint32_t) }, // Cell width
            { .constantID = 3, .offset = 3 * sizeof(uint32_t), .size = sizeof(uint32_t) }, // Cell height
            { .constantID = 4, .offset = 4 * sizeof(uint32_t), .size = sizeof(uint32_t) }, // Interpolated size
            { .constantID = 5, .offset = 5 * sizeof(uint32_t), .size = sizeof(uint32_t) }, // Cache Once length
            { .constantID = 6, .offset = 6 * sizeof(uint32_t), .size = sizeof(uint32_t) }, // Cache Flat length
            { .constantID = 7, .offset = 7 * sizeof(uint32_t), .size = sizeof(uint32_t) }, // Cache 2D Size
            { .constantID = 8, .offset = 8 * sizeof(uint32_t), .size = sizeof(uint32_t) } // Scale
        },
        .dataSize = sizeof(uint32_t) * 9,
        .pData = (uint32_t[]){
            stack->max_reg + 1,
            height,
            cell_width,
            cell_height,
            stack->interp_len,
            stack->cache_once_len,
            stack->cache_flat_len,
            stack->cache_2d_len,
            scale,
        }
    };
    ShaderCreateData create_data = {
        .buffers = buffers,
        .num_buffers = sizeof(buffers) / sizeof(buffers[0]),
        .desc_layouts = desc_layouts,
        .num_desc_layouts = sizeof(desc_layouts) / sizeof(desc_layouts[0]),
        .desc_updates = desc_updates,
        .num_desc_updates = sizeof(desc_updates) / sizeof(desc_updates[0]),
        .spec = &spec,
        .push_const_size = 16
    };
    struct timespec vkt_start, vkt_end;
    clock_gettime(TIME_UTC, &vkt_start);
    ShaderData* shader = (ShaderData*)malloc(sizeof(ShaderData));
    create_shader(state, (uint32_t*)eval_spv, eval_spv_len, &create_data, shader);
    mtx_lock(&state->fd_sem);
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
    // Copy noise_states staging buffer to GPU
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
    // Copy interp_indexes staging buffer to GPU
    vkCmdCopyBuffer(
        shader->vk_cmd,
        shader->shader_mem[8].vk_buf[0],
        shader->shader_mem[9].vk_buf[0],
        1,
        &(VkBufferCopy){ .size = sizeof(uint32_t) * stack->interp_len }
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
    VkBufferMemoryBarrier barriers[5] = {
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
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = shader->shader_mem[9].vk_buf[0],
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
        5, barriers,
        0, NULL
    );
    vkEndCommandBuffer(shader->vk_cmd);
    clock_gettime(TIME_UTC, &vkt_end);
    long long vk_time = (vkt_end.tv_sec - vkt_start.tv_sec) * 1000LL + (vkt_end.tv_nsec - vkt_start.tv_nsec) / 1000000LL;
    ShaderData** shaders = realloc(state->shaders, (state->shaders_len + 1) * sizeof(ShaderData*));
    if (shaders == NULL) {
        log_err("Shader data realloc returned null");
    } else {
        state->shaders = shaders;
    }
    state->shaders[state->shaders_len] = shader;
    EvalStack** final_density = realloc(state->final_density, (state->shaders_len + 1) * sizeof(EvalStack*));
    if (final_density == NULL) {
        log_err("Final density data realloc returned null");
    } else {
        state->final_density = final_density;
    }
    state->final_density[state->shaders_len] = stack;
    if (shaders != NULL && final_density != NULL)
        state->shaders_len++;
    if (state->shaders_len == 1) {
        VkFence fence;
        vkCreateFence(state->vk_device,
            &(VkFenceCreateInfo){
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
            },
            NULL, &fence
        );
        state->fd_fence = fence;
    }
    vkQueueSubmit(state->vk_queue, 1,
        &(VkSubmitInfo){
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &(shader->vk_cmd)
        },
        state->fd_fence
    );
    vkWaitForFences(state->vk_device, 1, &state->fd_fence, VK_TRUE, UINT64_MAX);
    mtx_unlock(&state->fd_sem);
    clock_gettime(TIME_UTC, &end);
    long long total_time = (end.tv_sec - start.tv_sec) * 1000LL + (end.tv_nsec - start.tv_nsec) / 1000000LL;

    log_info("Compiled compute shader for level \"%s\" in %lld ms (json: %lld ms, vulkan: %lld ms)", dim_str, total_time, json_time, vk_time);
    (*env)->ReleaseStringUTFChars(env, dimension, dim_str);
}

JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM* vm, void* reserved) {
    java_vm = vm;
    return JNI_VERSION_24;
}

JNIEXPORT jlong JNICALL
Java_com_endrf_LudicrousEngine_newInstance(JNIEnv* env, jclass cls) {
    InstanceState* state = create_instance(VK_QUEUE_COMPUTE_BIT);
    return (jlong)(intptr_t)state;
}

JNIEXPORT jobject JNICALL
Java_com_endrf_LudicrousEngine_fillNoiseAccel(JNIEnv* env, jclass cls, jlong instancePtr, jint fdIndex, jint x, jint z, jobject j_ret_type) {
    jint* j_ret = (*env)->GetIntArrayElements(env, j_ret_type, NULL);
    InstanceState* state = (InstanceState*)(intptr_t)instancePtr;
    if (state == NULL) return NULL;
    if (fdIndex > state->shaders_len - 1) {
        return NULL;
    }
    mtx_lock(&state->fd_sem);
    EvalStack* stack = state->final_density[fdIndex];
    int scale = stack->scale; // Should be odd
    int min_y = stack->min_y;
    int height = stack->height;
    int offset_factor = (16 * ((scale - 1) / 2));
    int x_origin = x - offset_factor;
    int z_origin = z - offset_factor;
    int width = 16 * scale;
    int size = height * width * width;
    int chunk_blocks = 256 * height;
    int cell_width = stack->cell_size_x << 2;
    int cell_height = stack->cell_size_y << 2;
    int xz_samples = (width / cell_width) + 1;
    int y_samples = (height / cell_height) + 1;
    int sample_size = xz_samples * xz_samples * y_samples;
    int largest_init_size = (width+1) * (width+1) * stack->cache_2d_len;

    // Get and return center chunk if cached
    uint64_t cached_chunk_index = get_cached_chunk_index(state->cache_map_len, x, z, fdIndex);
    float* cached_densities = cache_map_get(&state->cache_map, cached_chunk_index, x, z, fdIndex);
    if (cached_densities != NULL) {
        mtx_unlock(&state->fd_sem);
        j_ret[0] = cached_chunk_index; // indicates cached return
        (*env)->ReleaseIntArrayElements(env, j_ret_type, j_ret, 0);
        return (*env)->NewDirectByteBuffer(env, cached_densities, 256 * height * sizeof(float));
    } else {
        j_ret[0] = -1; // indicates computed return
    }
    (*env)->ReleaseIntArrayElements(env, j_ret_type, j_ret, 0);

    ShaderData* shader = state->shaders[fdIndex];
    vkResetCommandBuffer(shader->vk_cmd, 0);
    vkResetFences(state->vk_device, 1, &state->fd_fence);
    vkBeginCommandBuffer(shader->vk_cmd, &(VkCommandBufferBeginInfo){
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
    });
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
    // Reset global caches
    vkCmdPushConstants(
        shader->vk_cmd,
        shader->vk_pipeline_layout,
        VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        sizeof(int) * 4,
        &(int[]){x_origin, min_y, z_origin, 1}
    );
    vkCmdDispatch(shader->vk_cmd, (largest_init_size + 63) / 64, 1, 1);
    VkMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
    };
    vkCmdPipelineBarrier(
        shader->vk_cmd,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0,
        1, &barrier,
        0, NULL,
        0, NULL
    );
    // Compute interpolated samples
    vkCmdPushConstants(
        shader->vk_cmd,
        shader->vk_pipeline_layout,
        VK_SHADER_STAGE_COMPUTE_BIT,
        sizeof(int) * 3,
        sizeof(int),
        &(int[]){2}
    );
    vkCmdDispatch(shader->vk_cmd, (sample_size + 63) / 64, 1, 1);
    vkCmdPipelineBarrier(
        shader->vk_cmd,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0,
        1, &barrier,
        0, NULL,
        0, NULL
    );
    // Compute full chunk
    vkCmdPushConstants(
        shader->vk_cmd,
        shader->vk_pipeline_layout,
        VK_SHADER_STAGE_COMPUTE_BIT,
        sizeof(int) * 3,
        sizeof(int),
        &(int[]){3}
    );
    vkCmdDispatch(shader->vk_cmd, (size + 63) / 64, 1, 1);
    vkCmdPipelineBarrier(
        shader->vk_cmd,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, NULL,
        1, &(VkBufferMemoryBarrier){
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = shader->shader_mem[10].vk_buf[0],
            .offset = 0,
            .size = VK_WHOLE_SIZE
        },
        0, NULL
    );
    // Copy GPU output buffer to CPU
    vkCmdCopyBuffer(
        shader->vk_cmd,
        shader->shader_mem[10].vk_buf[0],
        shader->shader_mem[11].vk_buf[0],
        1,
        &(VkBufferCopy){ .srcOffset = 0, .dstOffset = 0, .size = sizeof(float) * size }
    );
    // Ensure copy with barrier
    vkCmdPipelineBarrier(
        shader->vk_cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_HOST_BIT,
        0,
        0, NULL,
        1, &(VkBufferMemoryBarrier){
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = shader->shader_mem[11].vk_buf[0],
            .offset = 0,
            .size = VK_WHOLE_SIZE
        },
        0, NULL
    );
    vkEndCommandBuffer(shader->vk_cmd);
    VkResult vk_res = vkQueueSubmit(state->vk_queue, 1,
        &(VkSubmitInfo){
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &(shader->vk_cmd)
        },
        state->fd_fence
    );
    if (vk_res != VK_SUCCESS)
        log_err("Error on vkQueueSubmit");

    float* final_densities;
    vk_res = vkWaitForFences(state->vk_device, 1, &state->fd_fence, VK_TRUE, UINT64_MAX);
    if (vk_res != VK_SUCCESS)
        log_err("Error on vkWaitForFences");
    vkMapMemory(
        state->vk_device,
        shader->shader_mem[11].vk_mem,
        0,
        sizeof(float) * size,
        0,
        (void**)&final_densities
    );
    float* densities = (float*)malloc(size * sizeof(float));
    memcpy(densities, final_densities, size * sizeof(float));
    vkUnmapMemory(state->vk_device, shader->shader_mem[11].vk_mem);
    mtx_unlock(&state->fd_sem);
    float* center_densities = (float*)malloc(256 * height * sizeof(float));

    for (int z_coord = z_origin, z_local = 0; z_coord <= z + offset_factor; z_coord += 16, z_local++) {
        for (int x_coord = x_origin, x_local = 0; x_coord <= x + offset_factor; x_coord += 16, x_local++) {
            if (x_coord == x && z_coord == z) {
                memcpy(center_densities,
                    densities + (z_local * chunk_blocks * scale) + (x_local * chunk_blocks),
                    256 * height * sizeof(float)
                );
                continue;
            }
            float* chunk = (float*)malloc(256 * height * sizeof(float));
            memcpy(
                chunk,
                densities + (z_local * chunk_blocks * scale) + (x_local * chunk_blocks),
                256 * height * sizeof(float)
            );
            cache_map_put(&state->cache_map, state->cache_map_len, x_coord, z_coord, fdIndex, chunk);
        }
    }

    free(densities);
    jobject buffer = (*env)->NewDirectByteBuffer(
        env,
        center_densities,
        256 * height * sizeof(float)
    );
    return buffer;
}

JNIEXPORT void JNICALL
Java_com_endrf_LudicrousEngine_releaseDensities(JNIEnv* env, jclass cls, jlong instancePtr, jobject buffer, jint chunk_index) {
    InstanceState* state = (InstanceState*)(intptr_t)instancePtr;
    if (chunk_index >= 0) {
        cache_map_release(&state->cache_map, chunk_index);
    } else {
        void* buf_ptr = (*env)->GetDirectBufferAddress(env, buffer);
        free(buf_ptr);
    }
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
    int num_interp_functions = state->final_density[0]->interp_len;
    int num_cache_once_functions = state->final_density[0]->cache_once_len;
    int num_cache_flat_functions = state->final_density[0]->cache_flat_len;
    int num_cache_2d_functions = state->final_density[0]->cache_2d_len;

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
                            ] = evaluate_stack(state->final_density[0], x + x_in_chunk, y_in_chunk + min_y, z + z_in_chunk, &eval_cache);
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
    double result = evaluate_stack(state->final_density[0], x, y, z, NULL);
    //printf("num interps %d\n", ((EvalStack*)state->data)->cache_once_len);
    //fflush(stdout);
    //clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    //printf("registers: %d\n", ((EvalStack*)state->data)->max_reg+1);
    //printf("result: %.30lf\n", result);
    //printf("time: %lld nanoseconds\n", (end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec));
    //fflush(stdout);
    return result;
}