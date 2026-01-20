#include "EngineCore/Rendering/render_thread.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Rendering/Resources/device.h"
#include "EngineCore/Rendering/gpu_resource.h"
#include "EngineCore/Rendering/render_context.h"
#include "EngineCore/Rendering/render_thread_controller.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/graphics_layer.h"
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

    VkDescriptorSetLayout UboLayout;
    VkDescriptorPool UniformDescriptorPool;

public:
    RenderThreadController(RenderThread *owner, VkDevice device, VkPhysicalDevice physicalDevice,
                           uint32_t graphicsQueueIndex, Logging::Logger *logger, int *frameParity,
                           VkDescriptorSetLayout uboLayout)
        : Owner(owner), Device(device), PhysicalDevice(physicalDevice), Logger(logger),
          FrameParity(frameParity), UboLayout(uboLayout)
    {
    }

    Runtime::CallbackResult Initialize()
    {
        // create the descriptor pool
        static constexpr VkDescriptorPoolSize poolSizes[] = {
            (VkDescriptorPoolSize){
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = MaxUniformBuffers,
            },
        };
        static constexpr VkDescriptorPoolCreateInfo globalPoolInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
            .maxSets = 1,
            .poolSizeCount = SDL_arraysize(poolSizes),
            .pPoolSizes = poolSizes,
        };

        // this isn't on the main thread but it's handy
        CHECK_VULKAN_MT(
            vkCreateDescriptorPool(Device, &globalPoolInfo, nullptr, &UniformDescriptorPool),
            "Failed to allocate bindless descriptor pool.");

        return Runtime::CallbackSuccess();
    }
};

RenderThread::RenderThread(Resources::Device *device) : m_Device(device)
{
}

Runtime::CallbackResult RenderThread::MtStart(Runtime::ServiceTable *services)
{
    m_DoneSemaphore = SDL_CreateSemaphore(1);
    m_ReadySemaphore = SDL_CreateSemaphore(0);

    if (m_DoneSemaphore == nullptr || m_ReadySemaphore == nullptr)
        return Runtime::Crash(__FILE__, __LINE__,
                              "Failed to create semaphores for rendering thread.");

    // create command pool
    VkCommandPoolCreateInfo cmdPoolInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = services->GraphicsLayer->m_Device.m_GraphicsQueueIndex,
    };
    CHECK_VULKAN_MT(vkCreateCommandPool(services->GraphicsLayer->m_Device.m_LogicalDevice,
                                        &cmdPoolInfo, nullptr, &m_CommandPool),
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

        CHECK_VULKAN_MT(vkAllocateCommandBuffers(services->GraphicsLayer->m_Device.m_LogicalDevice,
                                                 &allocInfo, &cmdBuffer),
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

        if (vkCreateSemaphore(services->GraphicsLayer->m_Device.m_LogicalDevice, &semaphoreInfo,
                              nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateFence(services->GraphicsLayer->m_Device.m_LogicalDevice, &fenceInfo, nullptr,
                          &inFlightFence) != VK_SUCCESS)
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
        CHECK_VULKAN_MT(vkCreateDescriptorPool(services->GraphicsLayer->m_Device.m_LogicalDevice,
                                               &flightPoolAllocInfo, nullptr, &flightPool),
                        "Failed to allocate bindless descriptor pool.");

        VkDescriptorSet flightDescSet = VK_NULL_HANDLE;
        VkDescriptorSetAllocateInfo flightDescSetInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = nullptr,
            .descriptorPool = flightPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &services->GraphicsLayer->m_RenderResources.PerFlightDescLayout,
        };
        CHECK_VULKAN_MT(vkAllocateDescriptorSets(services->GraphicsLayer->m_Device.m_LogicalDevice,
                                                 &flightDescSetInfo, &flightDescSet),
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
    RenderThreadController controller(this, m_Services->GraphicsLayer->m_Device.m_LogicalDevice,
                                      m_Services->GraphicsLayer->m_Device.m_PhysicalDevice,
                                      m_Services->GraphicsLayer->m_Device.m_GraphicsQueueIndex,
                                      m_Logger, &rtFrameParity,
                                      m_Services->GraphicsLayer->m_RenderResources.UboLayout);

    // initialize controller resources
    CHECK_CALLBACK_RT(controller.Initialize());

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
        CHECK_VULKAN_RT(vkWaitForFences(m_Services->GraphicsLayer->m_Device.m_LogicalDevice, 1,
                                        &currentCommand.InFlightFence, VK_TRUE, UINT32_MAX),
                        "Failed to wait for in-flight fence.");

        uint32_t imageIndex;
        CHECK_VULKAN_RT(vkAcquireNextImageKHR(m_Services->GraphicsLayer->m_Device.m_LogicalDevice,
                                              m_Services->GraphicsLayer->m_Swapchain.m_Swapchain,
                                              UINT64_MAX, currentCommand.ImageAvailableSemaphore,
                                              VK_NULL_HANDLE, &imageIndex),
                        "Failed to wait for a swapchain image.");
        Resources::SwapchainViewResources nextSwapchainView =
            m_Services->GraphicsLayer->m_Swapchain.m_ImageViewResources[imageIndex];

        // free resources
        m_GraphicsPipelines.PollFree(rtFrameParity, [&](VkPipeline pipeline) {
            vkDestroyPipeline(m_Device->m_LogicalDevice, pipeline, nullptr);
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
        CHECK_VULKAN_RT(vkQueueSubmit(m_Services->GraphicsLayer->m_Device.m_GraphicsQueue, 1,
                                      &submitInfo, currentCommand.InFlightFence),
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

        CHECK_VULKAN_RT(
            vkQueuePresentKHR(m_Services->GraphicsLayer->m_Device.m_PresentQueue, &presentInfo),
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

Runtime::CallbackResult RenderThread::MtUpdate()
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
    for (uint32_t updatedId : m_Services->GraphicsLayer->m_GraphicsPipelineUpdates)
    {
        m_GraphicsPipelines.Assign(
            updatedId, m_Services->GraphicsLayer->m_Shaders[updatedId].m_Pipeline, m_MtFrameParity,
            [](VkPipeline a, VkPipeline b) { return a == b; });
    }
    // geometries
    for (uint32_t updatedId : m_Services->GraphicsLayer->m_GeometryUpdates)
    {
        m_Geometries.Assign(updatedId, m_Services->GraphicsLayer->m_Geometries[updatedId],
                            m_MtFrameParity,
                            [](GpuGeometry a, GpuGeometry b) { return a.Buffer == b.Buffer; });
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
