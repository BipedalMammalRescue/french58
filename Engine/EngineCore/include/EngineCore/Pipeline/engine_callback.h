#pragma once

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

} // namespace Engine::Core::Pipeline