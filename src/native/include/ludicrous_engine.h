#ifndef LUDICROUS_ENGINE_H
#define LUDICROUS_ENGINE_H
#include <vulkan/vulkan.h>
#include "stack_builder.h"
#include "yyjson.h"

typedef struct ShaderData ShaderData;

typedef struct InstanceState {
    // Vulkan instance (might want this to be global eventually)
    VkInstance vk_instance;
    // Physical device for getting memory types
    VkPhysicalDevice vk_phys_device;
    // Index for the physical device's queue family
    uint32_t vk_queue_family_index;
    // Logical GPU Device
    VkDevice vk_device;
    // Queue for submitting command buffers
    VkQueue vk_queue;
    // Shader pipelines and related data
    ShaderData** shaders;
    size_t shaders_len;
    void* data;
} InstanceState;

Node* build_tree_from_json_recursive(yyjson_val* json);

#endif