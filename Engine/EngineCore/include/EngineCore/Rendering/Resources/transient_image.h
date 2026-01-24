#pragma once

#include "EngineCore/Rendering/Resources/geometry.h"
#include <vulkan/vulkan_core.h>

namespace Engine::Core::Logging {
class Logger;
}

namespace Engine::Core::Rendering {
class RenderThread;
}

namespace Engine::Core::Rendering::Resources {

// An image used in the middle of a frame and is never accessed outside of it.
// NOTE: this class represents images that are immediately allocated
class TransientImage
{
private:
    friend class RenderThread;

    VkImage m_Image;
    VmaAllocation m_Allocation;
    VkFormat m_Format;
    uint32_t m_Width;
    uint32_t m_Height;

public:
    VkResult Initialize(uint32_t width, uint32_t height, VkFormat format, VmaAllocator allocator,
                        VkImageUsageFlags usage, Logging::Logger *logger);
};

} // namespace Engine::Core::Rendering::Resources