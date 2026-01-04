#pragma once

#include "EngineCore/Logging/logger.h"
#include "EngineCore/Pipeline/engine_callback.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/module_assembly.h"
#include "EngineCore/Pipeline/module_definition.h"
#include "EngineCore/Pipeline/renderer_plugin_definition.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/event_manager.h"

#include <unordered_map>
#include <vector>

namespace Engine::Core::Runtime {

struct RootModuleState;
struct ServiceTable;

struct ModuleInstance
{
    Pipeline::ModuleDefinition Definition;
    void *State;
};

struct InstancedSynchronousCallback
{
    Pipeline::SynchronousCallbackDelegate Callback;
    void *InstanceState;
};

struct InstancedEventCallback
{
    Pipeline::EventCallbackDelegate Callback;
    void *InstanceState;
};

struct InstancedRendererPlugin
{
    Pipeline::RendererPluginDefinition Definition;
    void *PluginState;
    void *ModuleState;
};

class ModuleManager
{
private:
    ServiceTable *m_Services;
    Logging::Logger m_Logger;
    std::unordered_map<Pipeline::HashId, ModuleInstance> m_LoadedModules;

    // TODO: this needs to be refactored at some point, since this design doesn't work with dynamic
    // reloading
    std::vector<InstancedSynchronousCallback> m_PreupdateCallbacks;
    std::vector<InstancedSynchronousCallback> m_MidupdateCallbacks;
    std::vector<InstancedSynchronousCallback> m_PostupdateCallbacks;
    std::vector<InstancedSynchronousCallback> m_RenderCallbacks;
    std::vector<InstancedEventCallback> m_EventCallbacks;
    std::vector<EventSystemDelegate> m_EventSystems;

    // very very temporary solution!!!
    std::vector<InstancedRendererPlugin> m_RendererPlugins;

private:
    friend class GameLoop;
    CallbackResult LoadModules(const Pipeline::ModuleAssembly &modules, ServiceTable *services);
    CallbackResult UnloadModules();

public:
    ModuleManager(Logging::LoggerService *loggerService);
    ~ModuleManager();

    void *FindModuleMutable(const Pipeline::HashId &name);

    const void *FindModule(const Pipeline::HashId &name) const;
    const void *FindModule(const Pipeline::HashId &&name) const;

    template <typename TModule> const TModule *FindModule(const Pipeline::HashId &name) const
    {
        return static_cast<const TModule *>(FindModule(name));
    }

    template <typename TModule> const TModule *FindModule(const Pipeline::HashId &&name) const
    {
        return static_cast<const TModule *>(FindModule(name));
    }

    const RootModuleState *GetRootModule() const;

    inline const std::vector<InstancedRendererPlugin> *ListLoadedRendererPlugins() const
    {
        return &m_RendererPlugins;
    }
};

} // namespace Engine::Core::Runtime