#include "EngineCore/Rendering/Resources/geometry.h"
#include "EngineCore/Rendering/transfer_manager.h"
#include "SDL3/SDL_stdinc.h"
#include <vulkan/vulkan_core.h>

using namespace Engine::Core::Rendering::Resources;

VkResult Geometry::Initialize(TransferManager *transferManager, GeometryCreateInfo createInfo)
{
    // create a new buffer to hold all the data
    Transfer transfers[] = {createInfo.VertexBuffer, createInfo.IndexBuffer};
    VkValueResult<AllocatedBuffer> buffer = transferManager->Create(
        createInfo.StagingBuffer, transfers, SDL_arraysize(transfers),
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (buffer.Result != VK_SUCCESS)
        return buffer.Result;

    m_Buffer = buffer.Client.Buffer;
    m_Allocation = buffer.Client.Allocation;
    m_VertexBufferCount = 1;
    m_VertexBufferOffsets[0] = 0;
    m_IndexBufferOffset = createInfo.VertexBuffer.Length;
    m_IndexCount = createInfo.IndexCount;
    m_IndexType = createInfo.IndexType;
    return VK_SUCCESS;
}