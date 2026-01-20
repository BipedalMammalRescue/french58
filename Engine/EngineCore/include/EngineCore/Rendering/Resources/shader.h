#pragma once

#include "EngineCore/Logging/logger.h"
#include "EngineCore/Rendering/pipeline_setting.h"
#include "EngineCore/Rendering/render_target.h"
#include "EngineCore/Rendering/vertex_description.h"
#include <vulkan/vulkan_core.h>

namespace Engine::Core {
namespace Rendering {
class RenderThread;
}
namespace Runtime {
class GraphicsLayer;
}
} // namespace Engine::Core

namespace Engine::Core::Rendering::Resources {

struct ShaderCreationInfo
{
    VkPipelineLayout Layout;
    void *VertexCode;
    size_t VertShaderLength;
    void *FragmentCode;
    size_t FragShaderLength;
    const Rendering::VertexDescription *VertexSetting;
    const Rendering::PipelineSetting *PipelineSetting;
    const Rendering::ColorFormat *ColorAttachments;
    uint32_t ColorAttachmentCount;
    const Rendering::DepthPrecision *DepthPrecision;
};

class Shader
{
private:
    friend class Engine::Core::Rendering::RenderThread;
    friend class Engine::Core::Runtime::GraphicsLayer;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;

    bool Initialize(VkDevice device, VkFormat swapchainFormat, Logging::Logger *loggerj,
                    ShaderCreationInfo createInfo);

    inline void Dispose(VkDevice device)
    {
        vkDestroyPipeline(device, m_Pipeline, nullptr);
    }
};

} // namespace Engine::Core::Rendering::Resources