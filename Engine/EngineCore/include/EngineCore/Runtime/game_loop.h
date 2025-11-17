#pragma once

#include "EngineCore/Configuration/configuration_provider.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Logging/logger_service.h"
#include "EngineCore/Pipeline/asset_definition.h"
#include "EngineCore/Pipeline/component_definition.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/module_assembly.h"
#include "EngineCore/Runtime/asset_manager.h"
#include "EngineCore/Runtime/container_factory_service.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/event_manager.h"
#include "EngineCore/Runtime/event_writer.h"
#include "EngineCore/Runtime/graphics_layer.h"
#include "EngineCore/Runtime/heap_allocator.h"
#include "EngineCore/Runtime/input_manager.h"
#include "EngineCore/Runtime/network_layer.h"
#include "EngineCore/Runtime/service_table.h"
#include "EngineCore/Runtime/task_manager.h"
#include "EngineCore/Runtime/transient_allocator.h"
#include "EngineCore/Runtime/world_state.h"
#include "EngineCore/Pipeline/hash_id.h"

#include <unordered_map>

namespace Engine::Core::Logging {
class Logger;
}

namespace Engine::Core::Runtime {

class TaskManager;

class IGameLoopController
{
public:
    virtual CallbackResult Initialize() = 0;

    virtual CallbackResult LoadModules() = 0;
    virtual CallbackResult UnloadModules() = 0;

    virtual CallbackResult LoadEntity(Pipeline::HashId entityId) = 0;
    virtual CallbackResult ReloadAsset(Pipeline::HashId module, Pipeline::HashId type, Pipeline::HashId id) = 0;

    virtual CallbackResult PollAsyncIoEvents() = 0;

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
    class GameLoopController : public IGameLoopController
    {
    private:
        friend class GameLoop;

        GameLoop* m_Owner;

        Logging::LoggerService m_LoggerService;
        GraphicsLayer m_GraphicsLayer;
        WorldState m_WorldState;
        ModuleManager m_ModuleManager;
        EventManager m_EventManager;
        InputManager m_InputManager;
        NetworkLayer m_NetworkLayer;
        TaskManager m_TaskManager;
        TransientAllocator m_TransientAllocator;
        AssetManager m_AssetManager;
        HeapAllocator m_HeapAllocator;
        ContainerFactoryService m_ContainerFactory;
        
        ServiceTable m_Services;

        Logging::Logger m_TopLevelLogger;
        EventWriter m_EventWriter;

    public:
        GameLoopController(Pipeline::ModuleAssembly modules, Configuration::ConfigurationProvider configs, GameLoop* owner);

        const ServiceTable *GetServices() const override;
        CallbackResult Initialize() override;
        CallbackResult LoadModules() override;
        CallbackResult UnloadModules() override;
        CallbackResult LoadEntity(Pipeline::HashId entityId) override;
        CallbackResult ReloadAsset(Pipeline::HashId module, Pipeline::HashId type, Pipeline::HashId id) override;
        CallbackResult PollAsyncIoEvents() override;
        CallbackResult BeginFrame() override;
        CallbackResult Preupdate() override;
        CallbackResult EventUpdate() override;
        CallbackResult RenderPass() override;
        CallbackResult EndFrame() override;

        CallbackResult LoadAsset(Pipeline::HashIdTuple type, Pipeline::HashId assetId);
        CallbackResult LoadEntity(Pipeline::HashId entityId, ServiceTable services, Logging::Logger* logger);
    };

    // services
    Configuration::ConfigurationProvider m_ConfigurationProvider;
    Pipeline::ModuleAssembly m_Modules;
    std::unordered_map<Pipeline::HashIdTuple, Pipeline::ComponentDefinition> m_Components;
    std::unordered_map<Pipeline::HashIdTuple, Pipeline::AssetDefinition> m_AssetDefinitions;

    // events
    std::unordered_map<Pipeline::HashId, EventSystemInstance> m_EventSystems;

    CallbackResult RunCore(Pipeline::HashId initialEntityId);
    CallbackResult DiagnsoticModeCore(std::function<void(IGameLoopController*)> executor);

public:
    GameLoop(Pipeline::ModuleAssembly modules, const Configuration::ConfigurationProvider& configs);
    CallbackResult Run(Pipeline::HashId initialEntity);
    CallbackResult DiagnsoticMode(std::function<void(IGameLoopController*)> executor);

    bool AddEventSystem(EventSystemDelegate delegate, const char* userName);
    bool RemoveEventSystem(const char* userName);
};

}