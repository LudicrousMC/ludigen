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
    int shader_mem_len;
    // Layout for shader descriptor sets
    VkDescriptorSetLayout* vk_desc_set_layouts;
    int vk_desc_set_layouts_len;
    // Executable shader module
    VkShaderModule vk_shader_mod;
    // Layout for shader pipeline (expected descriptor sets)
    VkPipelineLayout vk_pipeline_layout;
    // Shader pipeline referencing shader module and pipeline layout
    VkPipeline vk_pipeline;
    VkDescriptorSet* vk_desc_sets;
    int vk_desc_sets_len;
    // Pool for allocating and managing descriptor sets
    VkDescriptorPool vk_desc_pool;
    // Pool for allocating and managing command buffers
    VkCommandPool vk_cmd_pool;
    // Command buffer for dispatching to queue
    VkCommandBuffer vk_cmd;
} ShaderData;

typedef struct ShaderBufferData {
    void* buf;
    size_t size;
    VkBufferUsageFlags usage;
    VkSharingMode sharing;
    VkMemoryPropertyFlags flags;
} ShaderBufferData;

typedef struct ShaderDescriptorLayout {
    VkDescriptorSetLayoutBinding* bindings;
    int num_bindings;
} ShaderDescriptorLayout;

typedef struct ShaderDescriptorSetUpdate {
    size_t desc_index;
    size_t* buf_indexes;
    size_t* mem_indexes;
    int desc_count;
    size_t set_index;
} ShaderDescriptorSetUpdate;

typedef struct ShaderCreateData {
    ShaderBufferData* buffers;
    int num_buffers;
    ShaderDescriptorLayout* desc_layouts;
    int num_desc_layouts;
    ShaderDescriptorSetUpdate* desc_updates;
    int num_desc_updates;
    VkSpecializationInfo* spec;
} ShaderCreateData;

int create_shader(InstanceState* state, uint32_t* spirv, size_t spirv_size, ShaderCreateData* data, ShaderData* shader);

#endif