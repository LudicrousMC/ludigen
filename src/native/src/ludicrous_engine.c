#include <vulkan/vulkan.h>
#include <jni.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ludicrous_engine.h"
#include "op_stack.h"
#include "op_types.h"
#include "vulkan_util.h"
#include "shader.h"
#include "logger.h"

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
    };
    return state;
}

typedef struct {
    StackFrame data[1024];
} StackBuffer;

typedef struct {
    double data[512];
} ConstArgsBuffer;

int main() {
    return 0;
}

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
    log_info("test 1");
    create_shader(state, spirvDataPtr);
}

JNIEXPORT jlong JNICALL
Java_com_endrf_LudicrousEngine_newInstance(JNIEnv* env, jclass cls) {
    InstanceState* state = create_instance(VK_QUEUE_COMPUTE_BIT);
    return (jlong)(intptr_t)state;
}