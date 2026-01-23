#pragma once

#include "EngineCore/Rendering/Lib/vk_mem_alloc.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>

namespace Engine::Core::Runtime {
class GraphicsLayer;
}

namespace Engine::Core::Rendering {

constexpr size_t MaxStorageBuffers = 65536;
constexpr size_t MaxImageSamplers = 65536;
constexpr size_t MaxUniformBuffers = 65536;

constexpr size_t PushConstantSize = 128;

uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties,
                        VkPhysicalDevice physicalDevice);

struct GpuImage
{
    VkImage Image;
    VkFormat Format;
    uint32_t Width;
    uint32_t Height;
    uint32_t MemoryType;
    uint32_t MemorySize;
};

GpuImage CreateImage(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width,
                     uint32_t height, VkFormat format, VkImageTiling tiling,
                     VkImageUsageFlags usage, VkMemoryPropertyFlags properties);

} // namespace Engine::Core::Rendering
