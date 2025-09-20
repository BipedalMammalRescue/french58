#pragma once

namespace Engine::Core::Runtime {
struct ServiceTable;
}

namespace Engine::Core::Pipeline {

enum class EngineCallbackStage
{
    Preupdate,
    ScriptUpdate,
    ModuleUpdate,
    Render
};

struct EngineCallback
{
    EngineCallbackStage Stage;
    void (*Callback)(Runtime::ServiceTable* services, void* moduleState);
};

}