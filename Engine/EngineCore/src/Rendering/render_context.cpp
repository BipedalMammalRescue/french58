#include "EngineCore/Rendering/render_context.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Runtime/graphics_layer.h"
#include <vulkan/vulkan_core.h>

using namespace Engine::Core;
using namespace Engine::Core::Rendering;

void Rendering::RenderContext::SetPipeline(uint32_t pipelineId)
{
    // find the pipeline
    if (pipelineId >= m_GraphicsLayer->m_GraphicsPipelines.size())
    {
        m_Logger->Verbose("Skipping invalid pipeline #{}.", pipelineId);
        return;
    }
    VkPipeline pipeline = m_GraphicsLayer->m_GraphicsPipelines[pipelineId];

    // bind it
    vkCmdBindPipeline(*m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void Rendering::RenderContext::Draw(uint32_t geometryId, void *pushConstantData,
                                    size_t pushConstantSize, void *uniformData, size_t uniformSize)
{
    // TODO:
}