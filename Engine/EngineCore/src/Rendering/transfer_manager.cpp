#include "EngineCore/Rendering/transfer_manager.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Rendering/Lib/vk_mem_alloc.h"
#include "EngineCore/Rendering/Resources/staging_buffer.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "common.h"
#include <vulkan/vulkan_core.h>

using namespace Engine::Core::Rendering;

Engine::Core::Runtime::CallbackResult TransferManager::Initialize(Logging::Logger *logger,
                                                                  VmaAllocator allocator,
                                                                  VkDevice device, VkQueue queue,
                                                                  uint32_t queueFamilyIndex)
{
    m_Logger = logger;
    m_Device = device;
    m_Queue = queue;
    m_Allocator = allocator;

    VkCommandPoolCreateInfo cmdPoolInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags =
            VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queueFamilyIndex,
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

VkValueResult<Resources::StagingBuffer> TransferManager::CreateStagingBuffer(size_t size)
{
    VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VmaAllocationCreateInfo allocationInfo{
        .flags = 0,
        .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
        .requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };

    // create the backing buffer
    VkBuffer newBuffer;
    VmaAllocation newAllocation;
    VkResult allocationResult = vmaCreateBuffer(m_Allocator, &bufferInfo, &allocationInfo,
                                                &newBuffer, &newAllocation, nullptr);
    if (allocationResult != VK_SUCCESS)
        return {.Result = allocationResult};

    // map memory
    void *mappedMemory = nullptr;
    VkResult mapResult = vmaMapMemory(m_Allocator, newAllocation, &mappedMemory);
    if (mapResult != VK_SUCCESS)
    {
        vmaDestroyBuffer(m_Allocator, newBuffer, newAllocation);
        return {.Result = mapResult};
    }

    // return
    Resources::StagingBuffer newStagingBuffer;
    newStagingBuffer.m_Size = size;
    newStagingBuffer.m_Buffer = newBuffer;
    newStagingBuffer.m_Allocation = newAllocation;
    newStagingBuffer.m_MappedMemory = mappedMemory;
    return {.Result = VK_SUCCESS, .Client = newStagingBuffer};
}

void Engine::Core::Rendering::TransferManager::DestroyStagingBuffer(
    Rendering::Resources::StagingBuffer buffer)
{
    vmaUnmapMemory(m_Allocator, buffer.m_Allocation);
    vmaDestroyBuffer(m_Allocator, buffer.m_Buffer, buffer.m_Allocation);
}

VkResult Engine::Core::Rendering::TransferManager::Upload(Resources::StagingBuffer src,
                                                          VkBuffer dst, Transfer *transfers,
                                                          size_t transferCount)
{
    // wait for the previous operation to be done
    vkWaitForFences(m_Device, 1, &m_TransferFence, VK_TRUE, UINT64_MAX);

    VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr,
    };
    VkResult beginCommandResult = vkBeginCommandBuffer(m_TransferCmdBuffer, &beginInfo);
    if (beginCommandResult != VK_SUCCESS)
        return beginCommandResult;

    for (size_t i = 0; i < transferCount; i++)
    {
        VkBufferCopy copyRegion{
            .srcOffset = transfers[i].SrcOffset,
            .dstOffset = transfers[i].DstOffset,
            .size = transfers[i].Length,
        };
        vkCmdCopyBuffer(m_TransferCmdBuffer, src.m_Buffer, dst, 1, &copyRegion);
    }

    vkEndCommandBuffer(m_TransferCmdBuffer);

    VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &m_TransferCmdBuffer,
    };
    return vkQueueSubmit(m_Queue, 1, &submitInfo, m_TransferFence);
}

VkValueResult<AllocatedBuffer> Engine::Core::Rendering::TransferManager::Create(
    Resources::StagingBuffer src, Transfer *transfers, size_t transferCount,
    VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryProps)
{
    size_t bufferSize = 0;
    for (Transfer *curTransfer = transfers; curTransfer < transfers + transferCount; curTransfer++)
    {
        bufferSize += curTransfer->Length;
    }

    // create a on-board buffer to hold all the data
    VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = bufferSize,
        .usage = bufferUsage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VmaAllocationCreateInfo allocationInfo{.usage = VMA_MEMORY_USAGE_AUTO,
                                           .requiredFlags = memoryProps};

    AllocatedBuffer newBuffer{VK_NULL_HANDLE, VK_NULL_HANDLE};

    VkResult allocationResult = vmaCreateBuffer(m_Allocator, &bufferInfo, &allocationInfo,
                                                &newBuffer.Buffer, &newBuffer.Allocation, nullptr);
    if (allocationResult != VK_SUCCESS)
        return {.Result = allocationResult};

    VkResult uploadResult = Upload(src, newBuffer.Buffer, transfers, transferCount);
    if (uploadResult != VK_SUCCESS)
    {
        vkDestroyBuffer(m_Device, newBuffer.Buffer, nullptr);
        vmaFreeMemory(m_Allocator, newBuffer.Allocation);
        return {.Result = uploadResult};
    }

    return {.Result = VK_SUCCESS, .Client = newBuffer};
}
