#include <stdint.h>
#include <vulkan/vulkan.h>
#include "ludicrous_engine.h"
#include "vulkan_util.h"

static void add_shader_memory(ShaderData shader, ShaderMemory mem) {
    shader->shader_mem = realloc(shader->shader_mem, ++shader->shader_mem_len * sizeof(ShaderMemory));
    shader->shader_mem[shader->shader_mem_len - 1] = mem;
}

static void create_buffer(InstanceState* state,
                          size_t shader_index,
                          VkBufferCreateInfo* buf_info,
                          VkMemoryPropertyFlags mem_props)
{
    VkBuffer* buf = (VkBuffer*)malloc(sizeof(VkBuffer));
    vkCreateBuffer(device, buf_info, NULL, buf);

    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(device, *buf, &req);

    VkDeviceMemory mem;
    vkAllocateMemory(device, &(VkMemoryAllocateInfo){
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = req.size,
            .memoryTypeIndex = findMemoryType(state->vk_phys_device, req.memoryTypeBits, mem_props),
        }, NULL, &mem);
    vkBindBufferMemory(device, *buf, mem, 0);

    add_shader_memory(state->shaders[shader_index], (ShaderMemory){
        .vk_mem = mem,
        .vk_buf = buf,
        .vk_buf_len = 1,
    });
}

static void add_buffer(InstanceState* state,
                       size_t shader_index,
                       size_t mem_index,
                       VkBufferCreateInfo* buf_info)
{
    VkBuffer buf;
    vkCreateBuffer(device, buf_info, NULL, &buf);

    ShaderData* shader = state->shaders[shader_index];

    VkDeviceMemory mem = shader->shader_mem[mem_index];
    vkBindBufferMemory(device, *buf, mem, 0);

    VkBuffer* shader_buffers = shader->shader_mem[mem_index].vk_buf;
    shader_buffers = (VkBuffer*)realloc(shader_buffers, sizeof(VkBuffer) * ++shader->shader_mem[mem_index]->vk_buf_len);
    shader_buffers[shader->shader_mem[mem_index].vk_buf_len - 1] = buf;
}

int create_shader(InstanceState* state, uint32_t* spirv) {
    return 0;
}