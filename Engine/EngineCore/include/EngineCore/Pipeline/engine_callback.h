#pragma once

#include "EngineCore/Runtime/crash_dump.h"
namespace Engine::Core::Runtime {
struct ServiceTable;
}

namespace Engine::Core::Pipeline {

enum class EngineCallbackStage
{
    Preupdate,
    EventUpdate,
    ModuleUpdate,
    Render
};

struct EngineCallback
{
    EngineCallbackStage Stage;
    Runtime::CallbackResult (*Callback)(Runtime::ServiceTable* services, void* moduleState);
};

}