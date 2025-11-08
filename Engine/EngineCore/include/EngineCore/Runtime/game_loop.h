#pragma once

#include "EngineCore/Configuration/configuration_provider.h"
#include "EngineCore/Pipeline/asset_definition.h"
#include "EngineCore/Pipeline/component_definition.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/module_assembly.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/event_manager.h"
#include "EngineCore/Runtime/service_table.h"
#include "EngineCore/Runtime/world_state.h"
#include "EngineCore/Pipeline/hash_id.h"

#include <unordered_map>

namespace Engine::Core::Logging {
class Logger;
}

namespace Engine::Core::Runtime {

class IGameLoopController
{
public:
    virtual CallbackResult Initialize() = 0;

    virtual CallbackResult LoadModules() = 0;
    virtual CallbackResult UnloadModules() = 0;

    virtual CallbackResult LoadEntity(Pipeline::HashId entityId) = 0;

    virtual CallbackResult BeginFrame() = 0;
    virtual CallbackResult Preupdate() = 0;
    virtual CallbackResult EventUpdate() = 0;
    virtual CallbackResult RenderPass() = 0;
    virtual CallbackResult EndFrame() = 0;

    virtual const ServiceTable* GetServices() const = 0;
};

class GameLoop
{
private:
    // services
    Configuration::ConfigurationProvider m_ConfigurationProvider;
    Pipeline::ModuleAssembly m_Modules;
    std::unordered_map<Pipeline::HashIdTuple, Pipeline::ComponentDefinition> m_Components;
    std::unordered_map<Pipeline::HashIdTuple, Pipeline::AssetDefinition> m_Assets;

    // events
    std::unordered_map<Pipeline::HashId, EventSystemInstance> m_EventSystems;

    CallbackResult RunCore(Pipeline::HashId initialEntityId);
    CallbackResult DiagnsoticModeCore(std::function<void(IGameLoopController*)> executor);

    // IO utilities (maybe move this to a service at sometime?)
    CallbackResult LoadEntity(Pipeline::HashId entityId, ServiceTable services, Logging::Logger* logger);

public:
    GameLoop(Pipeline::ModuleAssembly modules, const Configuration::ConfigurationProvider& configs);
    int Run(Pipeline::HashId initialEntity);
    CallbackResult DiagnsoticMode(std::function<void(IGameLoopController*)> executor);

    bool AddEventSystem(EventSystemDelegate delegate, const char* userName);
    bool RemoveEventSystem(const char* userName);
};

}