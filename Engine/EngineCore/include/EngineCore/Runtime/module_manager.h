#pragma once

#include "EngineCore/Logging/logger.h"
#include "EngineCore/Pipeline/engine_callback.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/module_assembly.h"
#include "EngineCore/Pipeline/module_definition.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/service_table.h"

#include <unordered_map>
#include <vector>

namespace Engine::Core::Pipeline {
struct ModuleDefinition;
}

namespace Engine::Core::Runtime {

struct RootModuleState;

struct ModuleInstance
{
    Pipeline::ModuleDefinition Definition;
    void* State;
};

struct InstancedSynchronousCallback
{
    CallbackResult (*Callback)(ServiceTable* services, void* moduleState);
    void* InstanceState;
};

struct InstancedEventCallback
{
    Runtime::CallbackResult (*Callback)(const Runtime::ServiceTable* services, void* moduleState, Runtime::EventStream* events);
    void* InstanceState;
};

class ModuleManager
{
private:
    ServiceTable* m_Services;
    Logging::Logger m_Logger;
    std::unordered_map<Pipeline::HashId, ModuleInstance> m_LoadedModules;
    std::vector<InstancedSynchronousCallback> m_RenderCallbacks;
    std::vector<InstancedEventCallback> m_EventCallbacks;

private:
    friend class GameLoop;
    CallbackResult LoadModules(const Pipeline::ModuleAssembly& modules, ServiceTable* services);

public:
    ~ModuleManager();
    const void* FindModule(const Pipeline::HashId& name) const;
    const void* FindModule(const Pipeline::HashId&& name) const;
    const RootModuleState* GetRootModule() const;
};

}