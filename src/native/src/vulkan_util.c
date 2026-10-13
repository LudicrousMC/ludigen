#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include "logger.h"

// Loads compiled spir-v shader from filepath
VkShaderModule createShaderModuleFromFile(VkDevice device,
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

uint32_t findMemoryType(VkPhysicalDevice physDev, uint32_t typeBits,
                        VkMemoryPropertyFlags props)
{
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(physDev, &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if ((typeBits & (1 << i)) &&
            (memProps.memoryTypes[i].propertyFlags & props) == props)
        {
            return i;
        }
    }
    log_err("Could not find suitable GPU memory type\n");
    exit(1);
}
