#pragma once

#include "EngineCore/Pipeline/hash_id.h"
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

    Pipeline::HashId m_CurrentModule;

    VkCommandBuffer *m_CommandBuffer;
    Runtime::GraphicsLayer *m_GraphicsLayer;
    Logging::Logger *m_Logger;

    uint32_t m_CurrentUniform = UINT32_MAX;

public:
    void SetPipeline(uint32_t pipelineId);
    void Draw(uint32_t geometryId, void *pushConstantData, size_t pushConstantSize,
              uint32_t uniformId);
};

} // namespace Engine::Core::Rendering