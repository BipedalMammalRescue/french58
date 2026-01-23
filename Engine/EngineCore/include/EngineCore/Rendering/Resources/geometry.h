#pragma once

#include "EngineCore/Rendering/Lib/vk_mem_alloc.h"
#include "EngineCore/Rendering/transfer_manager.h"
#include "SDL3/SDL_stdinc.h"
#include <vulkan/vulkan_core.h>

namespace Engine::Core::Rendering {
class RenderThread;
}

namespace Engine::Core::Rendering::Resources {

// Max number of vertex buffers allowed for a single geometry
static constexpr uint32_t MaxVertexBufferBindings = 8;

// Max number of total attributes for a single geometry
static constexpr uint32_t MaxVertexBufferAttributes = 16;

struct GeometryCreateInfo
{
    Rendering::Resources::StagingBuffer StagingBuffer;
    Rendering::Transfer VertexBuffer;
    Rendering::Transfer IndexBuffer;
    uint32_t IndexCount;
    VkIndexType IndexType;
};

class Geometry
{
private:
    friend class Engine::Core::Rendering::RenderThread;

    VkBuffer m_Buffer = VK_NULL_HANDLE;
    VmaAllocation m_Allocation = VK_NULL_HANDLE;

    uint32_t m_VertexBufferCount = UINT32_MAX;
    size_t m_VertexBufferOffsets[MaxVertexBufferBindings];

    uint32_t m_IndexBufferOffset = UINT32_MAX;
    uint32_t m_IndexCount = UINT32_MAX;
    VkIndexType m_IndexType;

public:
    bool Initialize(TransferManager *transferManager, GeometryCreateInfo createInfo);

    void Dispose();
};

} // namespace Engine::Core::Rendering::Resources