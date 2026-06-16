#include <vulkan/vulkan.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <op_stack.h>
#include <op_types.h>

#define ELEMENT_COUNT 1024
#define BUFFER_SIZE (ELEMENT_COUNT * sizeof(double))

typedef struct {
    uint32_t count;
    StackFrame data[1024];
} StackBuffer;

typedef struct {
    uint32_t count;
    double data[512];
} ConstArgsBuffer;

static uint32_t findMemoryType(VkPhysicalDevice physDev, uint32_t typeBits,
                                VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(physDev, &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if ((typeBits & (1 << i)) &&
            (memProps.memoryTypes[i].propertyFlags & props) == props)
            return i;
    }
    return UINT32_MAX;
}

// Loads compiled spir-v shader
static VkShaderModule createShaderModule(VkDevice device,
                                          const char *path) {
    FILE *f = fopen(path, "rb");
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    rewind(f);
    uint32_t *code = malloc(size);
    fread(code, 1, size, f);
    fclose(f);

    VkShaderModuleCreateInfo ci = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size,
        .pCode    = code,
    };
    VkShaderModule mod;
    vkCreateShaderModule(device, &ci, NULL, &mod);
    free(code);
    return mod;
}

// --- Buffer helpers ---

static void createBuffer(VkDevice device, VkPhysicalDevice physDev,
                          VkDeviceSize size, VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags memProps,
                          VkBuffer *buf, VkDeviceMemory *mem) {
    VkBufferCreateInfo bci = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size  = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    vkCreateBuffer(device, &bci, NULL, buf);

    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(device, *buf, &req);

    VkMemoryAllocateInfo ai = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = req.size,
        .memoryTypeIndex = findMemoryType(physDev, req.memoryTypeBits, memProps),
    };
    vkAllocateMemory(device, &ai, NULL, mem);
    vkBindBufferMemory(device, *buf, *mem, 0);
}

// --- Main ---

int main() {
    // 1. Instance
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion = VK_API_VERSION_1_2,
    };
    VkInstanceCreateInfo instCI = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
    };
    VkInstance instance;
    vkCreateInstance(&instCI, NULL, &instance);

    // 2. Physical device (pick first)
    uint32_t devCount = 1;
    VkPhysicalDevice physDev;
    vkEnumeratePhysicalDevices(instance, &devCount, &physDev);

    // 3. Find a compute queue family
    uint32_t queueFamilyIndex = UINT32_MAX;
    uint32_t qfCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physDev, &qfCount, NULL);
    VkQueueFamilyProperties *qfProps = malloc(qfCount * sizeof(*qfProps));
    vkGetPhysicalDeviceQueueFamilyProperties(physDev, &qfCount, qfProps);
    for (uint32_t i = 0; i < qfCount; i++) {
        if (qfProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            queueFamilyIndex = i;
            break;
        }
    }
    free(qfProps);

    // 4. Logical device + queue
    float qPriority = 1.0f;
    VkDeviceQueueCreateInfo qci = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queueFamilyIndex,
        .queueCount       = 1,
        .pQueuePriorities = &qPriority,
    };
    VkDeviceCreateInfo dci = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos    = &qci,
    };
    VkDevice device;
    vkCreateDevice(physDev, &dci, NULL, &device);

    VkQueue queue;
    vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);

    // 5. Buffers (host-visible for simplicity)
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
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT , hostVisible,
                 &constArgsBuf, &constArgsMem);
    createBuffer(device, physDev, BUFFER_SIZE,
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, hostVisible,
                 &outBuf, &outMem);

    // 6. Upload input data
    void *mapped1;
    StackBuffer stack = {0};
    stack.count = 7;
    vkMapMemory(device, stackMem, 0, sizeof(StackBuffer), 0, &mapped1);
    memcpy(mapped1, &stack, sizeof(StackBuffer));
    vkUnmapMemory(device, stackMem);
    void *mapped2;
    ConstArgsBuffer const_args = {0};
    const_args.count = 200;
    const_args.data[0] = 67.67f;
    vkMapMemory(device, constArgsMem, 0, sizeof(ConstArgsBuffer), 0, &mapped2);
    memcpy(mapped2, &const_args, sizeof(ConstArgsBuffer));
    vkUnmapMemory(device, constArgsMem);

    // 7. Descriptor set layout
    VkDescriptorSetLayoutBinding bindings[3] = {
        { .binding=0, .descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .descriptorCount=1, .stageFlags=VK_SHADER_STAGE_COMPUTE_BIT },
        { .binding=1, .descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
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

    // 8. Pipeline layout + compute pipeline
    VkPipelineLayoutCreateInfo plCI = {
        .sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts    = &descLayout,
    };
    VkPipelineLayout pipelineLayout;
    vkCreatePipelineLayout(device, &plCI, NULL, &pipelineLayout);

    VkShaderModule shaderMod = createShaderModule(device, "compute/eval.spv");

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
          .descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .pBufferInfo=&constArgsInfo },
        { .sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet=descSet,
          .dstBinding=2, .descriptorCount=1,
          .descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .pBufferInfo=&outInfo },
    };
    vkUpdateDescriptorSets(device, 3, writes, 0, NULL);

    // 11. Command buffer + dispatch
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
    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(device, &cbAI, &cmd);

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(cmd, &beginInfo);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                             pipelineLayout, 0, 1, &descSet, 0, NULL);
    // local_size_x=64, so groups = ELEMENT_COUNT / 64
    vkCmdDispatch(cmd, ELEMENT_COUNT / 64, 1, 1);
    vkEndCommandBuffer(cmd);

    // 12. Submit and wait
    VkSubmitInfo submitInfo = {
        .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers    = &cmd,
    };
    VkFenceCreateInfo fCI = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    VkFence fence;
    vkCreateFence(device, &fCI, NULL, &fence);
    vkQueueSubmit(queue, 1, &submitInfo, fence);
    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

    // 13. Read results
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
