#include "EngineCore/Rendering/render_thread.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Rendering/Resources/device.h"
#include "EngineCore/Rendering/Resources/geometry.h"
#include "EngineCore/Rendering/Resources/shader.h"
#include "EngineCore/Rendering/Resources/swapchain.h"
#include "EngineCore/Rendering/gpu_resource.h"
#include "EngineCore/Rendering/render_context.h"
#include "EngineCore/Rendering/render_thread_controller.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/module_manager.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_mutex.h"
#include "SDL3/SDL_thread.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>

using namespace Engine::Core::Rendering;
using namespace Engine::Core;

#define CHECK_CALLBACK_RT(expr)                                                                    \
    m_ExecutionResult = expr;                                                                      \
    if (m_ExecutionResult.has_value())                                                             \
    return 0

#define CHECK_VULKAN_MT(expression, error)                                                         \
    if (expression != VK_SUCCESS)                                                                  \
    return Runtime::Crash(__FILE__, __LINE__, error)

#define CHECK_VULKAN_RT(expression, error)                                                         \
    if (expression != VK_SUCCESS)                                                                  \
    {                                                                                              \
        m_ExecutionResult = Runtime::Crash(__FILE__, __LINE__, error);                             \
        return 0;                                                                                  \
    }

static int ThreadRoutine(void *state)
{
    return static_cast<RenderThread *>(state)->RtThreadRoutine();
}

class RenderThreadController : public IRenderThreadController
{
private:
    RenderThread *Owner;
    VkDevice Device;
    VkPhysicalDevice PhysicalDevice;
    uint32_t GraphicsQueueIndex;
    Logging::Logger *Logger;

    const int *FrameParity;

    VkDescriptorPool UniformDescriptorPool;

public:
    RenderThreadController(RenderThread *owner, VkDevice device, VkPhysicalDevice physicalDevice,
                           uint32_t graphicsQueueIndex, Logging::Logger *logger, int *frameParity)
        : Owner(owner), Device(device), PhysicalDevice(physicalDevice), Logger(logger),
          FrameParity(frameParity)
    {
    }
};

RenderThread::RenderThread(Resources::Device *device, Resources::Swapchain *swapchain)
    : m_Device(device), m_Swapchain(swapchain)
{
}

Runtime::CallbackResult RenderThread::MtStart(Runtime::ServiceTable *services,
                                              VkPipelineLayout sharedPipeline,
                                              VkDescriptorSetLayout perflightDescLayout)
{
    m_PipelineLayoutShared = sharedPipeline;

    m_DoneSemaphore = SDL_CreateSemaphore(1);
    m_ReadySemaphore = SDL_CreateSemaphore(0);

    if (m_DoneSemaphore == nullptr || m_ReadySemaphore == nullptr)
        return Runtime::Crash(__FILE__, __LINE__,
                              "Failed to create semaphores for rendering thread.");

    // create command pool
    VkCommandPoolCreateInfo cmdPoolInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = m_Device->m_GraphicsQueueSelection.queueFamilyIndex,
    };
    CHECK_VULKAN_MT(
        vkCreateCommandPool(m_Device->m_LogicalDevice, &cmdPoolInfo, nullptr, &m_CommandPool),
        "Failed to create Vulkan command pool.");

    // create the command buffers used in the flight
    for (uint32_t i = 0; i < 2; i++)
    {
        VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
        VkCommandBufferAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = m_CommandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        CHECK_VULKAN_MT(vkAllocateCommandBuffers(m_Device->m_LogicalDevice, &allocInfo, &cmdBuffer),
                        "Failed to create Vulkan command buffer");

        // a set of synchronizers
        VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
        VkFence inFlightFence = VK_NULL_HANDLE;

        VkSemaphoreCreateInfo semaphoreInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };

        VkFenceCreateInfo fenceInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };

        if (vkCreateSemaphore(m_Device->m_LogicalDevice, &semaphoreInfo, nullptr,
                              &imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateFence(m_Device->m_LogicalDevice, &fenceInfo, nullptr, &inFlightFence) !=
                VK_SUCCESS)
            return Runtime::Crash(__FILE__, __LINE__, "failed to create semaphores!");

        // create the double-buffered render target accessing
        VkDescriptorPool flightPool = VK_NULL_HANDLE;
        static constexpr VkDescriptorPoolSize flightPoolSizes[] = {
            (VkDescriptorPoolSize){
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 65536,
            },
        };
        static constexpr VkDescriptorPoolCreateInfo flightPoolAllocInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
            .maxSets = 1,
            .poolSizeCount = SDL_arraysize(flightPoolSizes),
            .pPoolSizes = flightPoolSizes,
        };
        CHECK_VULKAN_MT(vkCreateDescriptorPool(m_Device->m_LogicalDevice, &flightPoolAllocInfo,
                                               nullptr, &flightPool),
                        "Failed to allocate bindless descriptor pool.");

        VkDescriptorSet flightDescSet = VK_NULL_HANDLE;
        VkDescriptorSetAllocateInfo flightDescSetInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = nullptr,
            .descriptorPool = flightPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &perflightDescLayout,
        };
        CHECK_VULKAN_MT(
            vkAllocateDescriptorSets(m_Device->m_LogicalDevice, &flightDescSetInfo, &flightDescSet),
            "Failed to allocate double-buffered resource descriptor sets.");

        m_CommandsInFlight[i] = {
            .CommandBuffer = cmdBuffer,
            .ImageAvailableSemaphore = imageAvailableSemaphore,
            .InFlightFence = inFlightFence,
            .DescriptorPool = flightPool,
            .DescriptorSet = flightDescSet,
        };
    }

    m_Thread = SDL_CreateThread(ThreadRoutine, "RenderThread", this);
    if (m_Thread == nullptr)
        return Runtime::Crash(__FILE__, __LINE__,
                              std::string("Failed to create render thread, error: ") +
                                  SDL_GetError());

    return Runtime::CallbackSuccess();
}

int Engine::Core::Rendering::RenderThread::RtThreadRoutine()
{
    // note that the mt frame parity would be different from the rt one most of the time
    int rtFrameParity = 0;

    // create the controller
    // TODO: this interface needs some redo
    RenderThreadController controller(this, m_Device->m_LogicalDevice, m_Device->m_PhysicalDevice,
                                      m_Device->m_GraphicsQueueSelection.queueIndex, m_Logger,
                                      &rtFrameParity);

    while (true)
    {
        SDL_WaitSemaphore(m_ReadySemaphore);

        // update all renderer plugin states
        IRenderStateUpdateReader *reader = &m_EventStreams[rtFrameParity];
        for (Runtime::InstancedRendererPlugin instance : m_Plugins)
        {
            CHECK_CALLBACK_RT(instance.Definition.RtReadRenderStateUpdates(
                &controller, instance.PluginState, reader));
        }

        // render setup
        RenderSetupContext setupContext;
        for (Runtime::InstancedRendererPlugin instance : m_Plugins)
        {
            CHECK_CALLBACK_RT(instance.Definition.RtRenderSetup(&controller, instance.PluginState,
                                                                &setupContext));
        }

        // synchronization with the GPU
        CommandInFlight currentCommand = m_CommandsInFlight[rtFrameParity];
        CHECK_VULKAN_RT(vkWaitForFences(m_Device->m_LogicalDevice, 1, &currentCommand.InFlightFence,
                                        VK_TRUE, UINT32_MAX),
                        "Failed to wait for in-flight fence.");

        uint32_t imageIndex;
        CHECK_VULKAN_RT(vkAcquireNextImageKHR(m_Device->m_LogicalDevice, m_Swapchain->m_Swapchain,
                                              UINT64_MAX, currentCommand.ImageAvailableSemaphore,
                                              VK_NULL_HANDLE, &imageIndex),
                        "Failed to wait for a swapchain image.");
        Resources::SwapchainViewResources nextSwapchainView =
            m_Swapchain->m_ImageViewResources[imageIndex];

        // free resources
        m_GraphicsPipelines.PollFree(rtFrameParity, [&](Resources::Shader pipeline) {
            vkDestroyPipeline(m_Device->m_LogicalDevice, pipeline.m_Pipeline, nullptr);
        });

        // prepare command buffer for new frame
        CHECK_VULKAN_RT(vkResetCommandBuffer(currentCommand.CommandBuffer, 0),
                        "Failed to reset command buffer.");
        VkCommandBufferBeginInfo beginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pInheritanceInfo = nullptr,
        };
        CHECK_VULKAN_RT(vkBeginCommandBuffer(currentCommand.CommandBuffer, &beginInfo),
                        "Failed to begin command buffer.");

        const VkImageMemoryBarrier colorImageBarrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .image = nextSwapchainView.Image,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            }};
        vkCmdPipelineBarrier(currentCommand.CommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0,
                             nullptr, 1, &colorImageBarrier);

        // TODO: frame graph set up
        // TODO: frame graph execution

        // finish up the frame, submit, and present
        vkCmdEndRendering(currentCommand.CommandBuffer);
        const VkImageMemoryBarrier presentMemoryBarrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .image = nextSwapchainView.Image,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            }};
        vkCmdPipelineBarrier(currentCommand.CommandBuffer,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1,
                             &presentMemoryBarrier);
        CHECK_VULKAN_RT(vkEndCommandBuffer(currentCommand.CommandBuffer),
                        "Failed to end command buffer.");

        VkSemaphore waitSemaphores[] = {currentCommand.ImageAvailableSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

        VkSubmitInfo submitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = waitSemaphores,
            .pWaitDstStageMask = waitStages,
            .commandBufferCount = 1,
            .pCommandBuffers = &currentCommand.CommandBuffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &nextSwapchainView.RenderFinishSemaphore,
        };
        CHECK_VULKAN_RT(
            vkQueueSubmit(m_Device->m_GraphicsQueue, 1, &submitInfo, currentCommand.InFlightFence),
            "Failed to submit draw command buffer.");

        VkSwapchainKHR swapChains[] = {m_Swapchain->m_Swapchain};
        VkPresentInfoKHR presentInfo{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &nextSwapchainView.RenderFinishSemaphore,
            .swapchainCount = 1,
            .pSwapchains = swapChains,
            .pImageIndices = &imageIndex,
            .pResults = nullptr,
        };

        CHECK_VULKAN_RT(vkQueuePresentKHR(m_Device->m_PresentQueue, &presentInfo),
                        "Failed to queue swapchain presentation.");

        // flip the buffer index and frame in flight
        rtFrameParity = (rtFrameParity + 1) % 2;

        // signal main thread
        SDL_SignalSemaphore(m_DoneSemaphore);
    }
}

void RenderThread::EventStream::Write(void *data, size_t length)
{
    size_t writeOffset = EventStream.size();
    EventStream.resize(writeOffset + length);
    memcpy(EventStream.data() + writeOffset, data, length);
}

size_t RenderThread::EventStream::Read(void *buffer, size_t desiredLength)
{
    size_t actualRead = SDL_min(desiredLength, EventStream.size() - ReadPointer);
    memcpy(buffer, EventStream.data() + ReadPointer, actualRead);
    ReadPointer += actualRead;
    return actualRead;
}

Runtime::CallbackResult RenderThread::MtUpdate(
    UpdatedResources<Resources::Shader> shaderUpdates,
    UpdatedResources<Resources::Geometry> geometryUpdates)
{
    // wait for previous render thread work to be done
    SDL_WaitSemaphore(m_DoneSemaphore);

    // rebuild plugin table (this is in here because I plan to implement module state reloading at
    // some point)
    m_Plugins.clear();
    const std::vector<Runtime::InstancedRendererPlugin> *loadedPlugins =
        m_Services->ModuleManager->ListLoadedRendererPlugins();
    m_Plugins.reserve(loadedPlugins->size());
    for (Runtime::InstancedRendererPlugin plugin : *loadedPlugins)
    {
        m_Plugins.push_back(plugin);
    }

    // synchronize resources
    // shaders
    for (uint32_t *updatedId = shaderUpdates.Updates;
         updatedId < shaderUpdates.Updates + shaderUpdates.UpdateCount; updatedId++)
    {
        m_GraphicsPipelines.Assign(
            *updatedId, shaderUpdates.Source[*updatedId], m_MtFrameParity,
            [](Resources::Shader a, Resources::Shader b) { return a.m_Pipeline == b.m_Pipeline; });
    }
    // geometries
    for (uint32_t *updatedId = geometryUpdates.Updates;
         updatedId < geometryUpdates.Updates + geometryUpdates.UpdateCount; updatedId++)
    {
        m_Geometries.Assign(
            *updatedId, geometryUpdates.Source[*updatedId], m_MtFrameParity,
            [](Resources::Geometry a, Resources::Geometry b) { return a.m_Buffer == b.m_Buffer; });
    }

    // TODO: synchronize other resources

    // write updates to back buffer
    IRenderStateUpdateWriter *writer = &m_EventStreams[m_MtFrameParity];
    for (Runtime::InstancedRendererPlugin plugin : m_Plugins)
    {
        // prepend the length of this section to the
        size_t currentOffset = m_EventStreams[m_MtFrameParity].EventStream.size();
        m_EventStreams[m_MtFrameParity].EventStream.resize(currentOffset + sizeof(size_t));

        auto result =
            plugin.Definition.MtWriteRenderStateUpdates(m_Services, plugin.ModuleState, writer);
        if (result.has_value())
            return result;

        size_t newOffset = m_EventStreams[m_MtFrameParity].EventStream.size();
        size_t newLength = newOffset - currentOffset - sizeof(size_t);
        memcpy(m_EventStreams[m_MtFrameParity].EventStream.data() + currentOffset, &newLength,
               sizeof(size_t));
    }

    // signal the render thread
    SDL_SignalSemaphore(m_ReadySemaphore);

    // flip parity
    m_MtFrameParity = (m_MtFrameParity + 1) % 2;
    return Runtime::CallbackSuccess();
}

// RenderTarget GraphicsLayer::CreateRenderTarget(Rendering::RenderTargetUsage usage,
//                                                Rendering::RenderTargetSetting settings)
// {
//     VkFormat format;
//     uint32_t width;
//     uint32_t height;

//     // TODO: when do I need to change the tiling setting?
//     VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
//     VkImageUsageFlags gpuUsage;
//     VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

//     switch (usage)
//     {
//     case RenderTargetUsage::ColorTarget: {
//         switch (settings.Image.ColorTarget.ColorFormat)
//         {
//         case ColorFormat::UseSwapchain:
//             format = m_Swapchain.m_Format;
//             break;
//         }

//         if (settings.Image.ColorTarget.ScaleToSwapchain)
//         {
//             width = static_cast<uint32_t>(
//                 round(settings.Image.ColorTarget.Dimensions.Relative.WidthScale *
//                       m_Swapchain.m_Dimensions.width));
//             height = static_cast<uint32_t>(
//                 round(settings.Image.ColorTarget.Dimensions.Relative.HeightScale *
//                       m_Swapchain.m_Dimensions.height));
//         }
//         else
//         {
//             width = settings.Image.ColorTarget.Dimensions.Absolute.Width;
//             height = settings.Image.ColorTarget.Dimensions.Absolute.Height;
//         }

//         gpuUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
//     }
//     break;
//     case RenderTargetUsage::DepthBuffer: {
//         switch (settings.Image.DepthBuffer.Precision)
//         {
//         case Engine::Core::Rendering::DepthPrecision::D32:
//             format = VK_FORMAT_D32_SFLOAT;
//             break;
//         }

//         if (settings.Image.DepthBuffer.ScaleToSwapchain)
//         {
//             width = static_cast<uint32_t>(
//                 round(settings.Image.DepthBuffer.Dimensions.Relative.WidthScale *
//                       m_Swapchain.m_Dimensions.width));
//             height = static_cast<uint32_t>(
//                 round(settings.Image.DepthBuffer.Dimensions.Relative.HeightScale *
//                       m_Swapchain.m_Dimensions.height));
//         }
//         else
//         {
//             width = settings.Image.DepthBuffer.Dimensions.Absolute.Width;
//             height = settings.Image.DepthBuffer.Dimensions.Absolute.Height;
//         }
//     }
//     break;
//     }

//     if (settings.Sampler > Rendering::SamplerType::None)
//     {
//         gpuUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;
//     }

//     GpuImage image = CreateImage(m_Device.m_LogicalDevice, m_Device.m_PhysicalDevice, width,
//     height,
//                                  format, tiling, gpuUsage, properties);

//     return {
//         .Setting = settings,
//         .Image = image,
//         .View = VK_NULL_HANDLE,
//     };
// }

// // set up the critical path rendering targets (THESE ARE NOT BACKED BY MEMORY JUST YET)
// {
//     m_OpaqueColor.m_Id = 0;
//     m_RenderTargets[0] =
//         CreateRenderTarget(RenderTargetUsage::ColorTarget,
//                            (RenderTargetSetting){
//                                .Image =
//                                    {
//                                        .ColorTarget =
//                                            {
//                                                .ScaleToSwapchain = true,
//                                                .Dimensions = {.Relative = {1.0f, 1.0f}},
//                                                .ColorFormat = ColorFormat::UseSwapchain,
//                                            },
//                                    },
//                                .Sampler = Rendering::SamplerType::None,
//                            });

//     m_OpaqueDepth.m_Id = 1;
//     m_RenderTargets[1] =
//         CreateRenderTarget(Rendering::RenderTargetUsage::DepthBuffer,
//                            (RenderTargetSetting){
//                                .Image =
//                                    {
//                                        .ColorTarget =
//                                            {
//                                                .ScaleToSwapchain = true,
//                                                .Dimensions = {.Relative = {1.0f, 1.0f}},
//                                                .ColorFormat = ColorFormat::UseSwapchain,
//                                            },
//                                    },
//                                .Sampler = Rendering::SamplerType::None,
//                            });
// }