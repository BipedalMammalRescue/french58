#pragma once

#include "EngineCore/Logging/logger.h"
#include "EngineCore/Rendering/Resources/device.h"
#include "EngineCore/Rendering/Resources/geometry.h"
#include "EngineCore/Rendering/Resources/shader.h"
#include "EngineCore/Rendering/Resources/swapchain.h"
#include "EngineCore/Rendering/render_context.h"
#include "EngineCore/Rendering/render_target.h"
#include "EngineCore/Rendering/render_thread_controller.h"
#include "EngineCore/Rendering/rt_resource_manager.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/module_manager.h"
#include "EngineCore/Runtime/service_table.h"
#include "SDL3/SDL_mutex.h"
#include "SDL3/SDL_thread.h"
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Engine::Core::Runtime {
class GraphicsLayer;
}

namespace Engine::Core::Rendering {

template <typename T> struct UpdatedResources
{
    const T *Source;
    size_t SourceCount;
    uint32_t *Updates;
    size_t UpdateCount;
};

struct CommandInFlight
{
    VkCommandBuffer CommandBuffer;
    VkSemaphore ImageAvailableSemaphore;
    VkFence InFlightFence;

    VkDescriptorPool DescriptorPool;
    VkDescriptorSet DescriptorSet;
};

// Holds the synchronization mechanism for the render thread, its state and resources are
// created elsewhere, for the most part. The render thread is meant to take exclusive control of
// parts of the state of the game that's dedicated to rendering.
class RenderThread
{
    friend class RenderPassExecutionContext;
    friend class RenderStageExecutionContext;

private:
    Runtime::ServiceTable *m_Services;
    Logging::Logger *m_Logger;

private:
    // TODO: why are these injected at construction time?
    // injected
    Resources::Device *m_Device = nullptr;
    // injected
    Resources::Swapchain *m_Swapchain = nullptr;

    // injected dring initialization
    VkPipelineLayout m_PipelineLayoutShared = VK_NULL_HANDLE;

    // created
    VkCommandPool m_CommandPool = VK_NULL_HANDLE;
    // created
    CommandInFlight m_CommandsInFlight[2];
    // created
    SDL_Semaphore *m_ReadySemaphore = nullptr;
    // created
    SDL_Semaphore *m_DoneSemaphore = nullptr;
    // created
    SDL_Thread *m_Thread = nullptr;

    // result report
    Runtime::CallbackResult m_ExecutionResult = Runtime::CallbackSuccess();

    // used by main thread
    int m_MtFrameParity = 0;

    // double buffered resources
    RtResourceManager<Resources::Shader> m_GraphicsPipelines;
    RtResourceManager<Resources::Geometry> m_Geometries;

private:
    struct EventStream : public IRenderStateUpdateWriter, public IRenderStateUpdateReader
    {
        std::vector<uint8_t> EventStream;
        size_t ReadPointer;

        inline void Reset()
        {
            EventStream.clear();
            ReadPointer = 0;
        }

        void Write(void *data, size_t length) override;
        size_t Read(void *buffer, size_t desiredLength) override;
    };
    EventStream m_EventStreams[2];

    std::vector<Runtime::InstancedRendererPlugin> m_Plugins;

public:
    RenderThread(Resources::Device *device, Resources::Swapchain *swapchain);

    Runtime::CallbackResult MtStart(Runtime::ServiceTable *services,
                                    VkPipelineLayout sharedPipeline,
                                    VkDescriptorSetLayout perflightDescLayout);

    // Initiate a new frame on the render thread.
    Runtime::CallbackResult MtUpdate(UpdatedResources<Resources::Shader> shaderUpdates,
                                     UpdatedResources<Resources::Geometry> geometryUpdates);

    // Callable from the main thread, check if the thread is finished.
    bool MtTryJoin() const
    {
        auto status = SDL_GetThreadState(m_Thread);

        if (status != SDL_THREAD_COMPLETE)
            return false;

        SDL_WaitThread(m_Thread, nullptr);
        return true;
    }

    // Callable from the main thread, wait for the thread to be finished.
    void MtJoin() const
    {
        SDL_WaitThread(m_Thread, nullptr);
    }

    // Only callable after joining!
    Runtime::CallbackResult MtGetResult() const
    {
        return m_ExecutionResult;
    }

    int RtThreadRoutine();
};

} // namespace Engine::Core::Rendering