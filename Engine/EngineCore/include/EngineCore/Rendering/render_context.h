#pragma once

#include <cstddef>
#include <cstdint>
#include <vulkan/vulkan_core.h>

namespace Engine::Core::Runtime {
class GraphicsLayer;
}

namespace Engine::Core::Logging {
class Logger;
}

namespace Engine::Core::Rendering {

class RenderContext
{
private:
    friend class Engine::Core::Runtime::GraphicsLayer;

    VkCommandBuffer *m_CommandBuffer;
    Runtime::GraphicsLayer *m_GraphicsLayer;
    Logging::Logger *m_Logger;

public:
    void SetPipeline(uint32_t pipelineId);
    void Draw(uint32_t geometryId, void *pushConstantData, size_t pushConstantSize,
              void *uniformData, size_t uniformSize);
};

} // namespace Engine::Core::Rendering