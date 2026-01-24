#pragma once

#include "EngineCore/Rendering/Lib/vk_mem_alloc.h"
#include "EngineCore/Rendering/Resources/staging_buffer.h"
#include "EngineCore/Runtime/crash_dump.h"

#include <vulkan/vulkan_core.h>

namespace Engine::Core::Logging {
class Logger;
}

namespace Engine::Core::Rendering {

struct Transfer
{
    size_t SrcOffset;
    size_t DstOffset;
    size_t Length;
};

struct AllocatedBuffer
{
    VkBuffer Buffer;
    VmaAllocation Allocation;
};

template <typename TClient> struct VkValueResult
{
    VkResult Result;
    TClient Client;
};

class TransferManager
{
private:
    Logging::Logger *m_Logger;

    VmaAllocator m_Allocator = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    VkQueue m_Queue = VK_NULL_HANDLE;

    VkCommandPool m_TransferCmdPool = VK_NULL_HANDLE;
    VkCommandBuffer m_TransferCmdBuffer = VK_NULL_HANDLE;
    VkFence m_TransferFence = VK_NULL_HANDLE;

public:
    Runtime::CallbackResult Initialize(Logging::Logger *logger, VmaAllocator allocator,
                                       VkDevice device, VkQueue queue, uint32_t queueFamilyIndex);

    VkValueResult<Rendering::Resources::StagingBuffer> CreateStagingBuffer(size_t size);
    void DestroyStagingBuffer(Rendering::Resources::StagingBuffer buffer);

    // Upload data from a staging buffer to an arbitrary buffer
    VkResult Upload(Resources::StagingBuffer src, VkBuffer dest, Transfer *transfers,
                    size_t transferCount);

    VkValueResult<AllocatedBuffer> Create(Resources::StagingBuffer src, Transfer *transfers,
                                          size_t transferCount, VkBufferUsageFlags bufferUsage,
                                          VkMemoryPropertyFlags memoryProps);

    // wait for all previously scheduled transfer operations to finish
    inline void Join()
    {
        vkWaitForFences(m_Device, 1, &m_TransferFence, VK_TRUE, UINT64_MAX);
    }
};

} // namespace Engine::Core::Rendering