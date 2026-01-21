#pragma once

#include "EngineCore/Rendering/Lib/vk_mem_alloc.h"

#include <vulkan/vulkan_core.h>
namespace Engine::Core::Rendering {
class TransferManager;
}

namespace Engine::Core::Rendering::Resources {

class StagingBuffer
{
private:
    friend class Engine::Core::Rendering::TransferManager;

    size_t m_Size;
    VkBuffer m_Buffer = VK_NULL_HANDLE;
    VmaAllocation m_Allocation = VK_NULL_HANDLE;
    void *m_MappedMemory = VK_NULL_HANDLE;

public:
    inline size_t GetSize() const
    {
        return m_Size;
    }

    inline void *GetMapped() const
    {
        return m_MappedMemory;
    }
};

} // namespace Engine::Core::Rendering::Resources