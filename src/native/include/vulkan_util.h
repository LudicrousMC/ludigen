#ifndef VULKAN_UTIL_H
#define VULKAN_UTIL_H
#include <vulkan/vulkan.h>

VkShaderModule createShaderModuleFromFile(VkDevice device, const char *path);
VkShaderModule createShaderModuleFromBytes(VkDevice device, uint32_t* spirv, size_t size);
uint32_t findMemoryType(VkPhysicalDevice physDev, uint32_t typeBits, VkMemoryPropertyFlags props);
void createBuffer(VkDevice device, VkPhysicalDevice physDev,
                  VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags memProps,
                  VkBuffer *buf, VkDeviceMemory *mem);

#endif