#ifndef VULKAN_UTIL_H
#define VULKAN_UTIL_H
#include <vulkan/vulkan.h>

VkShaderModule createShaderModuleFromFile(VkDevice device, const char *path);
uint32_t findMemoryType(VkPhysicalDevice physDev, uint32_t typeBits, VkMemoryPropertyFlags props);

#endif