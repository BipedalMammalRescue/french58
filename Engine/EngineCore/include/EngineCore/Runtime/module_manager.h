#pragma once

#include "EngineCore/Logging/logger.h"
#include "EngineCore/Pipeline/engine_callback.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/module_assembly.h"
#include "EngineCore/Pipeline/module_definition.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/event_manager.h"
#include "EngineCore/Runtime/service_table.h"

#include <unordered_map>
#include <vector>

namespace Engine::Core::Runtime {

struct RootModuleState;

struct ModuleInstance
{
    Pipeline::ModuleDefinition Definition;
    void* State;
};

struct InstancedSynchronousCallback
{
    Pipeline::SynchronousCallbackDelegate Callback;
    void* InstanceState;
};

struct InstancedEventCallback
{
    Pipeline::EventCallbackDelegate Callback;
    void* InstanceState;
};

class ModuleManager
{
private:
    ServiceTable* m_Services;
    Logging::Logger m_Logger;
    std::unordered_map<Pipeline::HashId, ModuleInstance> m_LoadedModules;
    std::vector<InstancedSynchronousCallback> m_PreupdateCallbacks;
    std::vector<InstancedSynchronousCallback> m_MidupdateCallbacks;
    std::vector<InstancedSynchronousCallback> m_PostupdateCallbacks;
    std::vector<InstancedSynchronousCallback> m_RenderCallbacks;
    std::vector<InstancedEventCallback> m_EventCallbacks;
    std::vector<EventSystemDelegate> m_EventSystems;

private:
    friend class GameLoop;
    CallbackResult LoadModules(const Pipeline::ModuleAssembly& modules, ServiceTable* services);
    const void* FindModule(const Pipeline::HashId& name) const;
    const void* FindModule(const Pipeline::HashId&& name) const;

public:
    ~ModuleManager();

    template <typename TModule>
    const TModule* FindModule(const Pipeline::HashId& name) const
    {
        return static_cast<const TModule*>(FindModule(name));
    }

    template <typename TModule>
    const TModule* FindModule(const Pipeline::HashId&& name) const
    {
        return static_cast<const TModule*>(FindModule(name));
    }

    const RootModuleState* GetRootModule() const;
};

}