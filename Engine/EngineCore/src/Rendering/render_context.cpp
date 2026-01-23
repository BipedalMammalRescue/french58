#include "EngineCore/Rendering/render_context.h"
#include "EngineCore/Rendering/render_target.h"
#include "EngineCore/Rendering/render_thread.h"
#include <vulkan/vulkan_core.h>

using namespace Engine::Core;
using namespace Engine::Core::Rendering;

// void Rendering::RenderPassExecutionContext::SetPipeline(uint32_t pipelineId)
// {
//     // find the pipeline
//     if (pipelineId >= m_RenderThread->m_GraphicsPipelines.GetCount())
//     {
//         m_Logger->Verbose("Skipping invalid pipeline #{}.", pipelineId);
//         return;
//     }
//     VkPipeline pipeline = m_RenderThread->m_GraphicsPipelines.Get(pipelineId);

//     // bind it
//     vkCmdBindPipeline(*m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
// }

// void Rendering::RenderPassExecutionContext::Draw(uint32_t geometryId, void *pushConstantData,
//                                                  size_t pushConstantSize)
// {
//     // locate geometry
//     if (geometryId >= m_RenderThread->m_Geometries.GetCount())
//     {
//         m_Logger->Error("Skipping invalid geometry #{} (module {}).", geometryId,
//         m_CurrentModule); return;
//     }
//     GpuGeometry geometry = m_RenderThread->m_Geometries.Get(geometryId);

//     // push constant
//     if (pushConstantSize > PushConstantSize)
//     {
//         m_Logger->Error("Push constant length out of range for geometry #{} (module {}).",
//                         geometryId, m_CurrentModule);
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
//     vkCmdPushConstants(*m_CommandBuffer, m_RenderThread->m_PipelineLayoutShared,
//                        VK_SHADER_STAGE_ALL, 0, pushConstantSize, pushConstantData);

//     // draw!
//     vkCmdDrawIndexed(*m_CommandBuffer, geometry.IndexCount, 1, 0, 0, 0);
// }

// OutputColorTarget RenderPassConfigurator::WriteTo(ColorAttachmentTarget target)
// {
//     OutputColorTarget result;
//     result.m_Identifier = target.m_Id;
//     return result;
// }

// OutputDepthTarget RenderPassConfigurator::WriteTo(DepthAttachmentTarget target)
// {
//     OutputDepthTarget result;
//     result.m_Identifier = target.m_Id;
//     return result;
// }

Engine::Core::Rendering::RenderPassExecutionContext Engine::Core::Rendering::
    RenderStageExecutionContext::BeginRenderPass(OutputColorTarget *colorTargets,
                                                 size_t colorTargetCount,
                                                 std::optional<OutputDepthTarget> depthTarget)
{
    // TODO: this whole thing needs to be redesigned
    RenderPassExecutionContext result;

    // dynamic rendering begin
    m_AttachmentInfoBuffer.resize(colorTargetCount);
    m_ImageMemBarrierBuffer.resize(colorTargetCount);

    for (size_t i = 0; i < colorTargetCount; i++)
    {
        m_AttachmentInfoBuffer[i] = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = m_RenderTargets[colorTargets[i].m_Identifier].View,
            .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR,
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue =
                {
                    .color = {0.0f, 0.0f, 0.0f, 0.0f},
                },
        };

        m_ImageMemBarrierBuffer[i] = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,

            // NOTE: not sure if this is correct, but the scheduling should ensure that we only need
            // to wait on a write op here
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,

            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .image = m_RenderTargets[colorTargets[i].m_Identifier].Image.Image,
            .subresourceRange =
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
        };
    }

    VkRenderingAttachmentInfo depthAttachmentInfo;
    VkImageMemoryBarrier depthImageBarrier;
    if (depthTarget.has_value())
    {
        depthAttachmentInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = m_RenderTargets[depthTarget->m_Identifier].View,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue =
                {
                    .depthStencil = {.depth = 1.0f, .stencil = 0},
                },
        };

        depthImageBarrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,

            // NOTE: not sure if this is correct, but the assumption is the depth target only needs
            // to worry about previous writes visible
            .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,

            .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = m_RenderTargets[depthTarget->m_Identifier].Image.Image,
            .subresourceRange =
                {
                    .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
        };
    }

    VkRenderingInfo renderingInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = nullptr,
        .flags = 0,
        .renderArea =
            {
                .offset = {0, 0},
                .extent = m_SwapchainExtent,
            },
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = static_cast<uint32_t>(m_AttachmentInfoBuffer.size()),
        .pColorAttachments = m_AttachmentInfoBuffer.data(),
        // not using these for now
        .pDepthAttachment = depthTarget.has_value() ? &depthAttachmentInfo : nullptr,
        .pStencilAttachment = nullptr,
    };

    vkCmdPipelineBarrier(m_CommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr,
                         m_ImageMemBarrierBuffer.size(), m_ImageMemBarrierBuffer.data());

    if (depthTarget.has_value())
    {
        vkCmdPipelineBarrier(m_CommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr,
                             1, &depthImageBarrier);
    }

    vkCmdBeginRendering(m_CommandBuffer, &renderingInfo);

    return result;
}

void Engine::Core::Rendering::RenderStageExecutionContext::EndRenderPass()
{
    vkCmdEndRendering(m_CommandBuffer);
}
