#pragma once

#include "EngineCore/Rendering/Lib/vk_mem_alloc.h"
#include "EngineCore/Runtime/crash_dump.h"

#include <vulkan/vulkan_core.h>
namespace Engine::Core::Rendering {

class TransferManager
{
private:
    VmaAllocator m_Allocator = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    uint32_t m_QueueIndex = UINT32_MAX;

    VkCommandPool m_TransferCmdPool = VK_NULL_HANDLE;
    VkCommandBuffer m_TransferCmdBuffer = VK_NULL_HANDLE;

    // TODO: single fence, there has to be a way to get a smarter kind of notification
    VkFence m_TransferFence = VK_NULL_HANDLE;

public:
    Runtime::CallbackResult Initialize(VmaAllocator allocator, VkDevice device,
                                       uint32_t queueIndex);
};

} // namespace Engine::Core::Rendering