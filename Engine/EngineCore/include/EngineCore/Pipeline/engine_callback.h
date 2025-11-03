#pragma once

#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/event_stream.h"

namespace Engine::Core::Runtime {

struct ServiceTable;
class EventStream;

}

namespace Engine::Core::Pipeline {

enum class EngineCallbackStage
{
    Preupdate,
    Render
};

struct SynchronousCallback
{
    EngineCallbackStage Stage;
    Runtime::CallbackResult (*Callback)(Runtime::ServiceTable* services, void* moduleState);
};

struct EventCallback
{
    Runtime::CallbackResult (*Callback)(const Runtime::ServiceTable* services, void* moduleState, Runtime::EventStream eventStreams);
};

}