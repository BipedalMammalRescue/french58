#include "EngineCore/Rendering/render_context.h"
#include "EngineCore/Rendering/render_target.h"
#include "EngineCore/Rendering/render_thread.h"
#include "EngineCore/Runtime/graphics_layer.h"
#include <vulkan/vulkan_core.h>

using namespace Engine::Core;
using namespace Engine::Core::Rendering;

void Rendering::RenderPassExecutionContext::SetPipeline(uint32_t pipelineId)
{
    // find the pipeline
    if (pipelineId >= m_RenderThread->m_GraphicsPipelines.GetCount())
    {
        m_Logger->Verbose("Skipping invalid pipeline #{}.", pipelineId);
        return;
    }
    VkPipeline pipeline = m_RenderThread->m_GraphicsPipelines.Get(pipelineId);

    // bind it
    vkCmdBindPipeline(*m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void Rendering::RenderPassExecutionContext::Draw(uint32_t geometryId, void *pushConstantData,
                                                 size_t pushConstantSize)
{
    // locate geometry
    if (geometryId >= m_RenderThread->m_Geometries.GetCount())
    {
        m_Logger->Error("Skipping invalid geometry #{} (module {}).", geometryId, m_CurrentModule);
        return;
    }
    GpuGeometry geometry = m_RenderThread->m_Geometries.Get(geometryId);

    // push constant
    if (pushConstantSize > PushConstantSize)
    {
        m_Logger->Error("Push constant length out of range for geometry #{} (module {}).",
                        geometryId, m_CurrentModule);
        return;
    }

    // actually populate the command buffer
    VkBuffer duplicateBuffers[MaxVertexBufferBindings];
    for (size_t i = 0; i < MaxVertexBufferBindings; i++)
    {
        duplicateBuffers[i] = geometry.Buffer;
    }
    vkCmdBindVertexBuffers(*m_CommandBuffer, 0, geometry.VertexBufferCount, duplicateBuffers,
                           geometry.VertexBufferOffsets);
    vkCmdBindIndexBuffer(*m_CommandBuffer, geometry.Buffer, geometry.IndexBufferOffset,
                         geometry.IndexType);
    vkCmdPushConstants(*m_CommandBuffer, m_RenderThread->m_PipelineLayoutShared,
                       VK_SHADER_STAGE_ALL, 0, pushConstantSize, pushConstantData);

    // draw!
    vkCmdDrawIndexed(*m_CommandBuffer, geometry.IndexCount, 1, 0, 0, 0);
}

OutputColorTarget RenderPassConfigurator::WriteTo(ColorAttachmentTarget target)
{
    OutputColorTarget result;
    result.m_Identifier = target.m_Id;
    return result;
}

OutputDepthTarget RenderPassConfigurator::WriteTo(DepthAttachmentTarget target)
{
    OutputDepthTarget result;
    result.m_Identifier = target.m_Id;
    return result;
}
Engine::Core::Rendering::RenderPassExecutionContext Engine::Core::Rendering::
    RenderStageExecutionContext::BeginRenderPass(OutputColorTarget *colorTargets,
                                                 size_t colorTargetCount,
                                                 std::optional<OutputDepthTarget> depthTarget)
{
    RenderPassExecutionContext result;

    // TODO: dynamic rendering begin

    return result;
}
