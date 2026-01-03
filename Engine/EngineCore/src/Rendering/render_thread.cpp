#include "EngineCore/Rendering/render_thread.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_mutex.h"
#include "SDL3/SDL_thread.h"

using namespace Engine::Core::Rendering;
using namespace Engine::Core;

#define CHECK_CALLBACK_RT(expr)                                                                    \
    state->m_ExecutionResult = expr;                                                               \
    if (state->m_ExecutionResult.has_value())                                                      \
    return 0

RenderThread::RenderThread()
{
    m_DoneSemaphore = SDL_CreateSemaphore(1);
    m_ReadySemaphore = SDL_CreateSemaphore(0);
}

Runtime::CallbackResult RenderThread::MtStart(Runtime::ServiceTable *services)
{
    // TODO: load up the plugins

    m_Thread = SDL_CreateThread(ThreadRoutine, "RenderThread", this);
    if (m_Thread == nullptr)
        return Runtime::Crash(__FILE__, __LINE__,
                              std::string("Failed to create render thread, error: ") +
                                  SDL_GetError());

    return Runtime::CallbackSuccess();
}

int Engine::Core::Rendering::RenderThread::ThreadRoutine(void *userData)
{
    RenderThread *state = (RenderThread *)userData;
    int rtFrameParity = 0;

    while (true)
    {
        SDL_WaitSemaphore(state->m_ReadySemaphore);

        // update all renderer plugin states
        IRenderStateUpdateReader *reader = &state->m_EventStreams[rtFrameParity];
        for (RenderPluginInstance instance : state->m_Plugins)
        {
            CHECK_CALLBACK_RT(instance.ReadRenderStateUpdates(state->m_Services->GraphicsLayer,
                                                              instance.PluginState, reader));
        }

        // TODO: render setup

        // TODO: synchronization with the GPU

        // TODO: render execution

        // TODO: submit and present

        SDL_SignalSemaphore(state->m_DoneSemaphore);

        // flip the buffer index
        rtFrameParity = (rtFrameParity + 1) % 2;
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

    // write updates
    IRenderStateUpdateWriter *writer = &m_EventStreams[m_MtFrameParity];
    for (RenderPluginInstance plugin : m_Plugins)
    {
        // prepend the length of this section to the
        size_t currentOffset = m_EventStreams[m_MtFrameParity].EventStream.size();
        m_EventStreams[m_MtFrameParity].EventStream.resize(currentOffset + sizeof(size_t));

        auto result = plugin.WriteRenderStateUpdates(m_Services, plugin.ModuleState, writer);
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
