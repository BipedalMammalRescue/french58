#pragma once

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

class StagingBuffer
{
private:
    friend class Engine::Core::Runtime::GraphicsLayer;
    VkDevice m_Device = VK_NULL_HANDLE;
    VkBuffer m_Buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_Memory = VK_NULL_HANDLE;
    size_t m_Size = 0;

public:
    ~StagingBuffer()
    {
        if (m_Device == VK_NULL_HANDLE)
            return;

        if (m_Buffer != VK_NULL_HANDLE)
            vkDestroyBuffer(m_Device, m_Buffer, nullptr);

        if (m_Memory != VK_NULL_HANDLE)
            vkFreeMemory(m_Device, m_Memory, nullptr);
    }
};

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

// Max number of vertex buffers allowed for a single geometry
static constexpr uint32_t MaxVertexBufferBindings = 8;

// Max number of total attributes for a single geometry
static constexpr uint32_t MaxVertexBufferAttributes = 16;

struct GpuGeometry
{
    VkBuffer Buffer;
    VkDeviceMemory Memory;
    uint32_t VertexBufferCount;
    size_t VertexBufferOffsets[MaxVertexBufferBindings];
    uint32_t IndexBufferOffset;
    uint32_t IndexCount;
    VkIndexType IndexType;
};

struct CommandInFlight
{
    VkCommandBuffer CommandBuffer;
    VkSemaphore ImageAvailableSemaphore;
    VkFence InFlightFence;

    VkDescriptorPool DescriptorPool;
    VkDescriptorSet DescriptorSet;
};

} // namespace Engine::Core::Rendering
