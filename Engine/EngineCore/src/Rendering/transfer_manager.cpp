#include "EngineCore/Rendering/transfer_manager.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "common.h"
#include <vulkan/vulkan_core.h>

using namespace Engine::Core::Rendering;

Engine::Core::Runtime::CallbackResult TransferManager::Initialize(VmaAllocator allocator,
                                                                  VkDevice device,
                                                                  uint32_t queueIndex)
{
    m_Device = device;
    m_QueueIndex = queueIndex;
    m_Allocator = allocator;

    VkCommandPoolCreateInfo cmdPoolInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags =
            VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = m_QueueIndex,
    };
    CHECK_VULKAN(vkCreateCommandPool(m_Device, &cmdPoolInfo, nullptr, &m_TransferCmdPool),
                 "Failed to create Vulkan transfer command pool.");

    VkCommandBufferAllocateInfo cmdBufferInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = m_TransferCmdPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    CHECK_VULKAN(vkAllocateCommandBuffers(m_Device, &cmdBufferInfo, &m_TransferCmdBuffer),
                 "Failed to allocate transfer command buffer.");

    VkFenceCreateInfo fenceInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };
    CHECK_VULKAN(vkCreateFence(m_Device, &fenceInfo, nullptr, &m_TransferFence),
                 "Failed to create transfer fence.");

    return Runtime::CallbackSuccess();
}