#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include <stdio.h>
#include "ludicrous_engine.h"
#include "vulkan_util.h"
#include "shader.h"
#include "logger.h"

static ShaderMemory create_shader_memory(VkDeviceMemory mem, VkBuffer buf) {
    // Allocate for vk_buf
    VkBuffer* buf_ptr = (VkBuffer*)malloc(sizeof(VkBuffer));
    *buf_ptr = buf;
    return (ShaderMemory){
        .vk_mem = mem,
        .vk_buf = buf_ptr,
        .vk_buf_len = 1,
    };
}

static int map_shader_memory(InstanceState* state, VkDeviceMemory dest, void* src, size_t size) {
    void* mapped;
    vkMapMemory(state->vk_device, dest, 0, size, 0, &mapped);
    memcpy(mapped, src, size);
    vkUnmapMemory(state->vk_device, dest);
    return 0;
}

// Adds a ShaderMemory to the shader_mem field of ShaderData
static void add_shader_memory(ShaderData* shader, ShaderMemory mem) {
    shader->shader_mem = realloc(shader->shader_mem, ++shader->shader_mem_len * sizeof(ShaderMemory));
    shader->shader_mem[shader->shader_mem_len - 1] = mem;
}

static ShaderMemory create_buffer(InstanceState* state, VkBufferCreateInfo buf_info, VkMemoryPropertyFlags mem_props) {
    VkBuffer buf;
    vkCreateBuffer(state->vk_device, &buf_info, NULL, &buf);

    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(state->vk_device, buf, &req);

    VkDeviceMemory mem;
    vkAllocateMemory(state->vk_device, &(VkMemoryAllocateInfo){
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = req.size,
            .memoryTypeIndex = findMemoryType(state->vk_phys_device, req.memoryTypeBits, mem_props),
        }, NULL, &mem);
    vkBindBufferMemory(state->vk_device, buf, mem, 0);

    return create_shader_memory(mem, buf);
}

static void add_buffer(InstanceState* state,
                       ShaderData* shader,
                       size_t mem_index,
                       VkBufferCreateInfo buf_info)
{
    VkBuffer buf;
    vkCreateBuffer(state->vk_device, &buf_info, NULL, &buf);

    vkBindBufferMemory(state->vk_device, buf, shader->shader_mem[mem_index].vk_mem, 0);

    VkBuffer* shader_buffers = shader->shader_mem[mem_index].vk_buf;
    shader_buffers = (VkBuffer*)realloc(shader_buffers, sizeof(VkBuffer) * ++shader->shader_mem[mem_index].vk_buf_len);
    shader_buffers[shader->shader_mem[mem_index].vk_buf_len - 1] = buf;
}

int create_shader(InstanceState* state, uint32_t* spirv, size_t spirv_size, ShaderCreateData* data, ShaderData* shader) {
    //state->shaders = realloc(state->shaders, sizeof(ShaderData*) * ++state->shaders_len);
    // <-- Create Buffers and Map to GPU Memory -->
    // New Memory per buffer
    ShaderBufferData* buffers = data->buffers;
    ShaderMemory shader_mem[data->num_buffers];
    for (int i = 0; i < data->num_buffers; i++) {
        shader_mem[i] = create_buffer(state, (VkBufferCreateInfo)
            { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = buffers[i].size,
              .usage = buffers[i].usage, .sharingMode = buffers[i].sharing },
            buffers[i].flags
        );
        if (buffers[i].buf == NULL) continue;
        map_shader_memory(state, shader_mem[i].vk_mem, buffers[i].buf, buffers[i].size);
    }

    // <-- Create Descriptor Set Layouts for Shader Pipeline Layout and DS Allocation -->
    ShaderDescriptorLayout* desc_layouts = data->desc_layouts;
    VkDescriptorSetLayout desc_set_layouts[data->num_desc_layouts];
    uint32_t total_desc = 0;
    for (int i = 0; i < data->num_desc_layouts; i++) {
        vkCreateDescriptorSetLayout(state->vk_device, &(VkDescriptorSetLayoutCreateInfo)
            { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
              .bindingCount = desc_layouts[i].num_bindings, .pBindings = desc_layouts[i].bindings },
            NULL, &desc_set_layouts[i]
        );
        total_desc += desc_layouts[i].num_bindings;
    }

    // <-- Create Pipeline Layout from Descriptor Sets for Pipeline -->
    VkPipelineLayout pipeline_layout;
    vkCreatePipelineLayout(state->vk_device, &(VkPipelineLayoutCreateInfo)
        { .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
          .setLayoutCount = data->num_desc_layouts, .pSetLayouts = desc_set_layouts },
        NULL, &pipeline_layout
    );

    // <-- Create Shader Module from SPIR-V Bytes and Link to Shader Pipeline -->
    VkShaderModule shader_mod = createShaderModuleFromBytes(state->vk_device, spirv, spirv_size);
    VkPipeline pipeline;
    vkCreateComputePipelines(state->vk_device, VK_NULL_HANDLE, 1, &(VkComputePipelineCreateInfo)
        { .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
          .stage = { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                     .stage = VK_SHADER_STAGE_COMPUTE_BIT, .module = shader_mod, .pName = "main" },
          .layout = pipeline_layout },
        NULL, &pipeline
    );

    // <-- Create Pool for Descriptor Sets -->
    VkDescriptorPool desc_pool;
    vkCreateDescriptorPool(state->vk_device, &(VkDescriptorPoolCreateInfo)
        { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
          .maxSets = data->num_desc_layouts, .poolSizeCount = 1,
          .pPoolSizes = &(VkDescriptorPoolSize)
              { .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = total_desc } },
        NULL, &desc_pool
    );

    // <-- Allocate Descriptor Sets with Pool -->
    VkDescriptorSet desc_sets[data->num_desc_layouts];
    vkAllocateDescriptorSets(state->vk_device, &(VkDescriptorSetAllocateInfo)
        { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
          .descriptorPool = desc_pool, .descriptorSetCount = data->num_desc_layouts,
          .pSetLayouts = desc_set_layouts },
        desc_sets
    );

    ShaderDescriptorSetUpdate* desc_updates = data->desc_updates;
    VkWriteDescriptorSet desc_writes[data->num_desc_updates];
    VkDescriptorBufferInfo* info = (VkDescriptorBufferInfo*)malloc(total_desc * sizeof(VkDescriptorBufferInfo));
    for (int i = 0, j = 0, pre_j = 0; i < data->num_desc_updates; i++, pre_j = j) {
        ShaderDescriptorSetUpdate update = desc_updates[i];
        size_t set_index = update.set_index;
        size_t desc_index = update.desc_index;
        size_t desc_count = update.desc_count;
        VkDescriptorSetLayoutBinding desc = desc_layouts[set_index].bindings[desc_index];
        for (int k = 0; k < desc_count; k++, j++) {
            info[j] = (VkDescriptorBufferInfo){
                .buffer = shader_mem[update.mem_indexes[k]].vk_buf[update.buf_indexes[k]],
                .offset = 0,
                .range = VK_WHOLE_SIZE,
            };
        }
        desc_writes[i] = (VkWriteDescriptorSet){
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstBinding = desc.binding,
            .descriptorType = desc.descriptorType,
            .dstSet = desc_sets[set_index],
            .descriptorCount = desc_count,
            .pBufferInfo = info + pre_j,
        };
    }
    vkUpdateDescriptorSets(state->vk_device, sizeof(desc_writes) / sizeof(desc_writes[0]), desc_writes, 0, NULL);
    free(info);

    VkCommandPool cmd_pool;
    vkCreateCommandPool(state->vk_device, &(VkCommandPoolCreateInfo)
        { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
          .queueFamilyIndex = state->vk_queue_family_index },
        NULL, &cmd_pool
    );

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(state->vk_device, &(VkCommandBufferAllocateInfo)
        { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
          .commandPool = cmd_pool, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
          .commandBufferCount = 1 },
        &cmd
    );

    ShaderMemory* alloc_shader_mem = (ShaderMemory*)malloc(sizeof(shader_mem));
    memcpy(alloc_shader_mem, shader_mem, sizeof(shader_mem));
    VkDescriptorSetLayout* alloc_desc_set_layouts = (VkDescriptorSetLayout*)malloc(sizeof(desc_set_layouts));
    memcpy(alloc_desc_set_layouts, desc_set_layouts, sizeof(desc_set_layouts));
    VkDescriptorSet* alloc_desc_sets = (VkDescriptorSet*)malloc(sizeof(desc_sets));
    memcpy(alloc_desc_sets, desc_sets, sizeof(desc_sets));
    *shader = (ShaderData){
        .shader_mem = alloc_shader_mem,
        .shader_mem_len = sizeof(shader_mem) / sizeof(shader_mem[0]),
        .vk_desc_set_layouts = alloc_desc_set_layouts,
        .vk_desc_set_layouts_len = sizeof(desc_set_layouts) / sizeof(desc_set_layouts[0]),
        .vk_shader_mod = shader_mod,
        .vk_pipeline_layout = pipeline_layout,
        .vk_pipeline = pipeline,
        .vk_desc_sets = alloc_desc_sets,
        .vk_desc_sets_len = sizeof(desc_sets) / sizeof(desc_sets[0]),
        .vk_desc_pool = desc_pool,
        .vk_cmd_pool = cmd_pool,
        .vk_cmd = cmd,
    };
    return 0;
}