#include "EngineCore/Rendering/Resources/transient_image.h"
#include "EngineCore/Logging/logger.h"
#include <vulkan/vulkan_core.h>

using namespace Engine::Core::Rendering::Resources;
using namespace Engine::Core::Rendering;

VkResult TransientImage::Initialize(uint32_t width, uint32_t height, VkFormat format,
                                    VmaAllocator allocator, VkImageUsageFlags usage,
                                    VkDevice device, Logging::Logger *logger)
{
    m_Format = format;
    m_Width = width;
    m_Height = height;

    VkImageCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = {width, height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VmaAllocationCreateInfo allocationInfo{
        .flags = 0,
        .usage = VMA_MEMORY_USAGE_AUTO,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    VkResult result =
        vmaCreateImage(allocator, &createInfo, &allocationInfo, &m_Image, &m_Allocation, nullptr);

    if (result != VK_SUCCESS)
        return result;

    VkImageViewCreateInfo viewInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .image = m_Image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };

    return vkCreateImageView(device, &viewInfo, nullptr, &m_View);
}

void Engine::Core::Rendering::Resources::TransientImage::Dispose(VmaAllocator allocator,
                                                                 VkDevice device)
{
    if (m_View != VK_NULL_HANDLE)
    {
        vkDestroyImageView(device, m_View, nullptr);
    }

    if (m_Allocation != VK_NULL_HANDLE)
    {
        vmaDestroyImage(allocator, m_Image, m_Allocation);
    }

    m_Image = VK_NULL_HANDLE;
    m_Allocation = VK_NULL_HANDLE;
    m_View = VK_NULL_HANDLE;
}
