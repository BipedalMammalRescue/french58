#include "EngineCore/Rendering/render_context.h"
#include "EngineCore/Rendering/render_target.h"
#include "EngineCore/Runtime/graphics_layer.h"
#include <vulkan/vulkan_core.h>

using namespace Engine::Core;
using namespace Engine::Core::Rendering;

// void Rendering::RenderPassExecutionContext::SetPipeline(uint32_t pipelineId)
// {
//     // find the pipeline
//     if (pipelineId >= m_GraphicsLayer->m_GraphicsPipelines.size())
//     {
//         m_Logger->Verbose("Skipping invalid pipeline #{}.", pipelineId);
//         return;
//     }
//     VkPipeline pipeline = m_GraphicsLayer->m_GraphicsPipelines[pipelineId];

//     // bind it
//     vkCmdBindPipeline(*m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
// }

// void Rendering::RenderPassExecutionContext::Draw(uint32_t geometryId, void *pushConstantData,
//                                                  size_t pushConstantSize, uint32_t uniformId)
// {
//     // locate geometry
//     if (geometryId >= m_GraphicsLayer->m_Geometries.size())
//     {
//         m_Logger->Error("Skipping invalid geometry #{} (module {}).", geometryId,
//         m_CurrentModule); return;
//     }
//     GpuGeometry geometry = m_GraphicsLayer->m_Geometries[geometryId];

//     // push constant
//     if (pushConstantSize > PushConstantSize)
//     {
//         m_Logger->Error("Push constant length out of range for geometry #{} (module {}).",
//                         geometryId, m_CurrentModule);
//         return;
//     }

//     // set uniform
//     if (uniformId < UINT32_MAX && uniformId >= m_GraphicsLayer->m_UniformBuffers.size())
//     {
//         m_Logger->Error("Skipping invalid uniform buffer #{} (module {}).", uniformId,
//                         m_CurrentModule);
//         return;
//     }

//     // actually populate the command buffer
//     VkBuffer duplicateBuffers[MaxVertexBufferBindings];
//     for (size_t i = 0; i < MaxVertexBufferBindings; i++)
//     {
//         duplicateBuffers[i] = geometry.Buffer;
//     }
//     vkCmdBindVertexBuffers(*m_CommandBuffer, 0, geometry.VertexBufferCount, duplicateBuffers,
//                            geometry.VertexBufferOffsets);
//     vkCmdBindIndexBuffer(*m_CommandBuffer, geometry.Buffer, geometry.IndexBufferOffset,
//                          geometry.IndexType);
//     vkCmdPushConstants(*m_CommandBuffer, m_GraphicsLayer->m_RenderResources.GlobalPipelineLayout,
//                        VK_SHADER_STAGE_ALL, 0, pushConstantSize, pushConstantData);

//     // bind uniform buffer
//     if (m_CurrentUniform != uniformId)
//     {
//         VkDescriptorSet uniformDescSet =
//         m_GraphicsLayer->m_UniformBuffers[uniformId].Get().DescSet;
//         vkCmdBindDescriptorSets(*m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
//                                 m_GraphicsLayer->m_RenderResources.GlobalPipelineLayout, 0, 1,
//                                 &uniformDescSet, 0, nullptr);
//         m_CurrentUniform = uniformId;
//     }

//     // draw!
//     vkCmdDrawIndexed(*m_CommandBuffer, geometry.IndexCount, 1, 0, 0, 0);
// }

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
