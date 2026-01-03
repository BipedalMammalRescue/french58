#pragma once

#include "EngineCore/Rendering/render_context.h"
#include "EngineCore/Rendering/render_thread_controller.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/event_stream.h"

namespace Engine::Core::Runtime {

struct ServiceTable;
class EventStream;
class ITaskScheduler;

} // namespace Engine::Core::Runtime

namespace Engine::Core::Pipeline {

enum class SynchronousCallbackStage
{
    Preupdate,
    MidUpdate,
    PostUpdate,
    Render
};

using SynchronousCallbackDelegate = Runtime::CallbackResult (*)(Runtime::ServiceTable *services,
                                                                void *moduleState);

struct SynchronousCallback
{
    SynchronousCallbackStage Stage;
    SynchronousCallbackDelegate Callback;
};

using EventCallbackDelegate = Runtime::CallbackResult (*)(const Runtime::ServiceTable *services,
                                                          Runtime::ITaskScheduler *scheduler,
                                                          void *moduleState,
                                                          Runtime::EventStream eventStreams);

struct EventCallback
{
    EventCallbackDelegate Callback;
};

using MtRenderUpdateCallback =
    Runtime::CallbackResult (*)(Runtime::ServiceTable *services, void *moduleState,
                                Rendering::IRenderStateUpdateWriter *writer);

using RtRenderUpdateCallback =
    Runtime::CallbackResult (*)(Rendering::IRenderThreadController *renderer, void *pluginState,
                                Rendering::IRenderStateUpdateReader *reader);

using RtRenderSetupCallback =
    Runtime::CallbackResult (*)(const Rendering::IRenderThreadController *renderer,
                                void *pluginState, Rendering::RenderSetupContext *context);
using RtRenderExecuteCallback =
    Runtime::CallbackResult (*)(const Rendering::IRenderThreadController *renderer,
                                void *pluginState, Rendering::RenderExecutionContext *context);
} // namespace Engine::Core::Pipeline