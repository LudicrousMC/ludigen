#ifndef SHADER_H
#define SHADER_H
#include <vulkan/vulkan.h>

typedef struct InstanceState InstanceState;

typedef struct ShaderMemory {
    // Device memory binding for storage buffers
    VkDeviceMemory vk_mem;
    // Storage buffers for compute data (contains size and usage information)
    VkBuffer* vk_buf;
    size_t vk_buf_len;
} ShaderMemory;

typedef struct ShaderData {
    // Shader data for allocated GPU memory and related buffers
    ShaderMemory* shader_mem;
    size_t shader_mem_len;
    // Layout for shader descriptor sets
    VkDescriptorSetLayout vk_desc_set_layout;
    // Executable shader modules
    VkShaderModule* vk_shader_mod;
    // Layout for shader pipeline (expected descriptor sets)
    VkPipelineLayout vk_pipeline_layout;
    // Shader pipeline referencing shader module and pipeline layout
    VkPipeline vk_pipeline;
    // Pool for allocating and managing descriptor sets
    VkDescriptorPool vk_desc_pool;
    // Pool for allocating and managing command buffers
    VkCommandPool vk_cmd_pool;
} ShaderData;


int create_shader(InstanceState* state, uint32_t* spirv);

#endif