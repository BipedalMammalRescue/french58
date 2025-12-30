#include "EngineCore/Rendering/gpu_resource.h"
#include <vulkan/vulkan_core.h>

using namespace Engine::Core::Rendering;

uint32_t Engine::Core::Rendering::FindMemoryType(uint32_t typeFilter,
                                                 VkMemoryPropertyFlags properties,
                                                 VkPhysicalDevice physicalDevice)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if (typeFilter & (1 << i) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }

    return UINT32_MAX;
}

Engine::Core::Rendering::GpuImage Engine::Core::Rendering::CreateImage(
    VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height,
    VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties)
{
    GpuImage image{
        .Width = width,
        .Height = height,
    };

    VkImageCreateInfo imageInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent =
            {
                .width = width,
                .height = height,
                .depth = 1,
            },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    if (vkCreateImage(device, &imageInfo, nullptr, &image.Image) != VK_SUCCESS)
        return image;

    VkMemoryRequirements memRequirements{};
    vkGetImageMemoryRequirements(device, image.Image, &memRequirements);

    image.MemoryType = FindMemoryType(memRequirements.memoryTypeBits, properties, physicalDevice);
    image.MemorySize = memRequirements.size;
    return image;
}
