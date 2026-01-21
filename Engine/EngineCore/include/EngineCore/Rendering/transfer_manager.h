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

class TransferManager
{
private:
    Logging::Logger *m_Logger;

    VmaAllocator m_Allocator = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    VkQueue m_Queue = VK_NULL_HANDLE;

    VkCommandPool m_TransferCmdPool = VK_NULL_HANDLE;
    VkCommandBuffer m_TransferCmdBuffer = VK_NULL_HANDLE;

    // TODO: single fence, there has to be a way to get a smarter kind of notification
    VkFence m_TransferFence = VK_NULL_HANDLE;

public:
    Runtime::CallbackResult Initialize(Logging::Logger *logger, VmaAllocator allocator,
                                       VkDevice device, VkQueue queue, uint32_t queueFamilyIndex);

    std::optional<Rendering::Resources::StagingBuffer> CreateStagingBuffer(size_t size);
    void DestroyStagingBuffer(Rendering::Resources::StagingBuffer buffer);

    // TODO: eventually make it asynchronous
    // Upload data from a staging buffer to an arbitrary buffer
    bool Upload(Resources::StagingBuffer src, VkBuffer dest, Transfer *transfers,
                size_t transferCount);
};

} // namespace Engine::Core::Rendering