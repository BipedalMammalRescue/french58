#pragma once

#include "EngineCore/Logging/logger.h"
#include "EngineCore/Pipeline/renderer_plugin_definition.h"
#include "EngineCore/Rendering/gpu_resource.h"
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

// Holds the synchronization mechanism for the render thread, its state and resources are
// created elsewhere, for the most part. The render thread is meant to take exclusive control of
// parts of the state of the game that's dedicated to that thread.
class RenderThread
{
private:
    Runtime::ServiceTable *m_Services;
    Logging::Logger *m_Logger;

private:
    VkCommandPool m_CommandPool = VK_NULL_HANDLE;
    CommandInFlight m_CommandsInFlight[2];

    VkDescriptorPool m_DescriptorPool;

    SDL_Semaphore *m_ReadySemaphore = nullptr;
    SDL_Semaphore *m_DoneSemaphore = nullptr;
    SDL_Thread *m_Thread = nullptr;

    // result report
    Runtime::CallbackResult m_ExecutionResult = Runtime::CallbackSuccess();

    // used by main thread
    int m_MtFrameParity = 0;

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
    RenderThread();

    Runtime::CallbackResult MtStart(Runtime::ServiceTable *services);

    // Initiate a new frame on the render thread.
    Runtime::CallbackResult MtUpdate();

    inline int MtGetFrameParity() const
    {
        return m_MtFrameParity;
    }

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