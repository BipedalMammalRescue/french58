#include "EngineCore/Runtime/game_loop.h"

#include "EngineCore/Configuration/configuration_provider.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Logging/logger_service.h"
#include "EngineCore/Pipeline/asset_definition.h"
#include "EngineCore/Pipeline/asset_enumerable.h"
#include "EngineCore/Pipeline/engine_callback.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/module_assembly.h"
#include "EngineCore/Pipeline/module_definition.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/event_writer.h"
#include "EngineCore/Runtime/graphics_layer.h"
#include "EngineCore/Runtime/input_manager.h"
#include "EngineCore/Runtime/network_layer.h"
#include "EngineCore/Runtime/service_table.h"
#include "EngineCore/Runtime/world_state.h"
#include "EngineCore/Runtime/module_manager.h"
#include "EngineUtils/String/hex_strings.h"
#include "EngineCore/Runtime/event_manager.h"
#include "EngineCore/Runtime/task_manager.h"

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <fstream>
#include <md5.h>
#include <string>

using namespace Engine::Core::Runtime;

GameLoop::GameLoop(Pipeline::ModuleAssembly modules, const Configuration::ConfigurationProvider& configs) : 
    m_ConfigurationProvider(configs),
    m_Modules(modules)
{
    // build component table
    for (size_t i = 0; i < modules.ModuleCount; i++)
    {
        Pipeline::ModuleDefinition module = modules.Modules[i];
        for (size_t j = 0; j < module.ComponentCount; j ++)
        {
            Pipeline::ComponentDefinition component = module.Components[j];
            Pipeline::HashIdTuple tuple { module.Name.Hash, component.Name.Hash };
            m_Components[tuple] = component;
        }
    }

    // build asset table
    for (size_t i = 0; i < modules.ModuleCount; i++)
    {
        Pipeline::ModuleDefinition module = modules.Modules[i];
        for (size_t j = 0; j < module.AssetsCount; j ++)
        {
            Pipeline::AssetDefinition asset = module.Assets[j];
            Pipeline::HashIdTuple tuple { module.Name.Hash, asset.Name.Hash };
            m_AssetDefinitions[tuple] = asset;
        }
    }
}

bool GameLoop::AddEventSystem(EventSystemDelegate delegate, const char* userName)
{
    Pipeline::HashId inId = md5::compute(userName);
    return m_EventSystems.try_emplace(inId, EventSystemInstance{delegate, userName}).second;
}

bool GameLoop::RemoveEventSystem(const char* userName)
{
    auto iterator = m_EventSystems.find(md5::compute(userName));
    if (iterator == m_EventSystems.end())
        return false;

    m_EventSystems.erase(iterator);
    return true;
}

CallbackResult GameLoop::DiagnsoticModeCore(std::function<void(IGameLoopController*)> executor)
{
    GameLoopController controller(m_ConfigurationProvider, this);

    executor(&controller);

    return CallbackSuccess();
}

CallbackResult GameLoop::DiagnsoticMode(std::function<void(IGameLoopController*)> executor)
{
    // initialize sdl
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
        std::string error("SDL initialization failed, error: ");
        error.append(SDL_GetError());
        return Crash(__FILE__, __LINE__, error);
	}

    auto result = DiagnsoticModeCore(executor);

    // quit SDL
    SDL_Quit();
    return result;
}

CallbackResult GameLoop::RunCore(Pipeline::HashId initialEntityId)
{
    GameLoopController controller(m_ConfigurationProvider, this);

    CallbackResult gameresult = controller.Initialize();
    if (gameresult.has_value())
        return gameresult;

    gameresult = controller.LoadModules();
    if (gameresult.has_value())
        return gameresult;

    // load the first scene
    gameresult = controller.LoadEntity(initialEntityId);
    if (gameresult.has_value())
        return gameresult;

    // game loop
    bool quit = false;
    while (!quit)
    {
        gameresult = controller.BeginFrame();
        if (gameresult.has_value())
            return gameresult;

        quit = controller.m_InputManager.m_QuitRequested;

        // pre-update
        gameresult = controller.Preupdate();
        if (gameresult.has_value())
            return gameresult;

        // event loop
        gameresult = controller.EventUpdate();
        if (gameresult.has_value())
            return gameresult;

        // render pass
        gameresult = controller.RenderPass();
        if (gameresult.has_value())
            return gameresult;
        
        // last step in the update loop
        gameresult = controller.EndFrame();
        if (gameresult.has_value())
            return gameresult;
    }

    controller.UnloadModules();
    return CallbackSuccess();
}

CallbackResult GameLoop::Run(Pipeline::HashId initialEntityId) 
{
    // initialize sdl
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
        std::string error("SDL initialization failed, error: ");
        error.append(SDL_GetError());
        return Crash(__FILE__, __LINE__, error);
	}

    // allow crash to persistent outside the game loop
    CallbackResult gameError = RunCore(initialEntityId);

    // quit SDL
    SDL_Quit();

    return gameError;
}

class StreamAssetEnumerator : public Engine::Core::Pipeline::IAssetEnumerator
{
private:
    int m_Cursor = -1;
    int m_Count = 0;
    std::istream* m_Source = nullptr;
    std::ifstream m_AssetFile;
    Engine::Core::Pipeline::HashId m_AssetId = { {0} };
    char m_NameBuffer[43] = "CD0ED230BD87479C61DB68677CAA9506.bse_asset";

public:
    StreamAssetEnumerator(std::istream* source)
    {
        source->read((char*)&m_Count, sizeof(int));
        m_Source = source;
    }

    size_t Count() override 
    {
        return m_Count;
    };

    bool MoveNext() override 
    {
        if (m_Cursor + 1 >= m_Count)
            return false;

        m_Cursor ++;

        // close the original file
        if (m_AssetFile.is_open())
        {
            m_AssetFile.close();
        }

        // open the new file
        m_Source->read((char*)m_AssetId.Hash.data(), 16);
        Engine::Utils::String::BinaryToHex(16, m_AssetId.Hash.data(), m_NameBuffer);
        m_AssetFile.open(m_NameBuffer, std::ios::binary);

        return m_AssetFile.is_open();
    }

    Engine::Core::Pipeline::RawAsset GetCurrent() override 
    {
        return { &m_AssetFile, m_AssetId };
    }
};

static bool CheckMagicWord(unsigned int target, std::istream* input)
{
    unsigned int getWord = 0;
    input->read((char*)&getWord, sizeof(unsigned int));
    return target == getWord;
}

static std::string EntityLoadingError(Engine::Core::Pipeline::HashId entityId, const char* reason)
{
    std::string errorMessage;
    errorMessage.append("Error loading entity ");
    char entityIdStr[33] {0};
    Engine::Utils::String::BinaryToHex(16, entityId.Hash.data(), entityIdStr);
    errorMessage.append(entityIdStr);
    errorMessage.append(" reason: ");
    errorMessage.append(reason);
    return errorMessage;
}

// CallbackResult GameLoop::GameLoopController::LoadAsset(Pipeline::HashIdTuple assetGroupId, Pipeline::HashId assetId)
// {
//     m_TopLevelLogger.Information("Loading asset: {id}, ({module}:{type})", { assetId, assetGroupId.First, assetGroupId.Second });

//     auto targetAssetType = m_Owner->m_Assets.find(assetGroupId);
//     if (targetAssetType == m_Owner->m_Assets.end())
//     {
//         m_TopLevelLogger.Warning("Module state not found for asset: {module}:{type}, assets will be skipped", {assetGroupId.First, assetGroupId.Second});
//         return CallbackSuccess();
//     }

//     auto targetModuleState = m_Services.ModuleManager->m_LoadedModules.find(assetGroupId.First);
//     if (targetModuleState == m_Services.ModuleManager->m_LoadedModules.end())
//     {
//         m_TopLevelLogger.Warning("Module state not found: {module}.", {assetGroupId.First});
//         return CallbackSuccess();
//     }

//     AssetManagement::AssetLoadingContext context {};
//     context.AssetId = assetId;
//     context.AssetGroupId = assetGroupId;

//     // contextualize
//     auto contextualizeResult = targetAssetType->second.Contextualize(&m_Services, targetModuleState->second.State, assetId, context);
//     if (contextualizeResult.has_value())
//         return contextualizeResult;

//     // load
//     switch (context.Buffer.Type)
//     {
//     case AssetManagement::LoadBufferType::TransientBuffer:
//         // open file

//     case AssetManagement::LoadBufferType::ModuleBuffer:
//         break;
//     default:
//         return Crash(__FILE__, __LINE__, "Error loading asset: unrecognized context buffer type returned from contextualizer. asset id = {TODO}");
//     }
// }

CallbackResult GameLoop::GameLoopController::LoadEntity(Pipeline::HashId entityId)
{
    m_TopLevelLogger.Information("Loading entity {}", entityId);

    char pathBuffer[] = "CD0ED230BD87479C61DB68677CAA9506.bse_entity";
    Utils::String::BinaryToHex(16, entityId.Hash.data(), pathBuffer);

    std::ifstream entityFile;
    entityFile.open(pathBuffer);

    if (!entityFile.is_open())
    {
        static const char errorMessage[] = "Entity file can't be opened.";
        m_TopLevelLogger.Fatal(errorMessage);
        return Crash(__FILE__, __LINE__, EntityLoadingError(entityId, errorMessage));
    }

    // read the asset section
    if (!CheckMagicWord(0xCCBBFFF1, &entityFile))
    {
        static const char errorMessage[] = "Entity magic word for asset section mismatch.";
        m_TopLevelLogger.Fatal(errorMessage);
        return Crash(__FILE__, __LINE__, EntityLoadingError(entityId, errorMessage));
    }

    int assetGroupCount = 0;
    entityFile.read((char*)&assetGroupCount, sizeof(int));
    for (int assetGroupIndex = 0; assetGroupIndex < assetGroupCount; assetGroupIndex ++)
    {
        Pipeline::HashIdTuple assetGroupId;
        entityFile.read((char*)&assetGroupId, 32);

        auto targetAssetType = m_Owner->m_AssetDefinitions.find(assetGroupId);
        if (targetAssetType == m_Owner->m_AssetDefinitions.end())
        {
            m_TopLevelLogger.Fatal("Module state not found for asset: {}:{}.", assetGroupId.First, assetGroupId.Second);
            return Crash(__FILE__, __LINE__, EntityLoadingError(entityId, "Asset definition not found."));
        }

        auto targetModuleState = m_Services.ModuleManager->m_LoadedModules.find(assetGroupId.First);
        if (targetModuleState == m_Services.ModuleManager->m_LoadedModules.end())
        {
            m_TopLevelLogger.Fatal("Module state not found: {}.", assetGroupId.First);
            return Crash(__FILE__, __LINE__, EntityLoadingError(entityId, "Module state not found for asset."));
        }

        StreamAssetEnumerator enumerator(&entityFile);
        targetAssetType->second.Load(&enumerator, &m_Services, targetModuleState->second.State);
    }

    // read the entities
    if (!CheckMagicWord(0xCCBBFFF2, &entityFile) || !m_Services.WorldState->LoadEntities(&entityFile))
    {
        static const char errorMessage[] = "Entity magic word for entity section mismatch.";
        m_TopLevelLogger.Fatal(errorMessage);
        return Crash(__FILE__, __LINE__, EntityLoadingError(entityId, errorMessage));
    }

    // read the components
    if (!CheckMagicWord(0xCCBBFFF3, &entityFile))
    {
        static const char errorMessage[] = "Entity magic word for component section mismatch.";
        m_TopLevelLogger.Fatal(errorMessage);
        return Crash(__FILE__, __LINE__, EntityLoadingError(entityId, errorMessage));
    }

    int componentGroupCount = 0;
    entityFile.read((char*)&componentGroupCount, sizeof(int));
    for (int componentGroupIndex = 0; componentGroupIndex < componentGroupCount; componentGroupIndex ++)
    {
        Pipeline::HashIdTuple componentGroupId;
        entityFile.read((char*)&componentGroupId, 32);
        
        auto targetComponent = m_Owner->m_Components.find(componentGroupId);
        if (targetComponent == m_Owner->m_Components.end())
        {
            m_TopLevelLogger.Fatal("Module state not found for component: {}:{}.", componentGroupId.First, componentGroupId.Second);
            return Crash(__FILE__, __LINE__, EntityLoadingError(entityId, "Asset definition not found."));
        }

        auto targetModuleState = m_Services.ModuleManager->m_LoadedModules.find(componentGroupId.First);
        if (targetModuleState == m_Services.ModuleManager->m_LoadedModules.end())
        {
            m_TopLevelLogger.Fatal("Module state not found: {}.", componentGroupId.First);
            return Crash(__FILE__, __LINE__, EntityLoadingError(entityId, "Module state not found for asset."));
        }

        int componentCount = 0;
        entityFile.read((char*)&componentCount, sizeof(int));
        targetComponent->second.Load(componentCount, &entityFile, &m_Services, targetModuleState->second.State);
    }

    m_TopLevelLogger.Verbose("Loaded {} asset groups, {} component groups.", assetGroupCount, componentGroupCount);
    return CallbackSuccess();
}


static const char* TopLevelLogChannels[] = { "GameLoop" };

GameLoop::GameLoopController::GameLoopController(Engine::Core::Configuration::ConfigurationProvider configs, GameLoop* owner)
    : m_LoggerService(configs),
    m_GraphicsLayer(&configs, &m_LoggerService),
    m_WorldState(&configs),
    m_ModuleManager(&m_LoggerService),
    m_EventManager(&m_LoggerService),
    m_InputManager(),
    m_NetworkLayer(&m_LoggerService),
    m_TaskManager(&m_Services, &m_LoggerService, configs.WorkerCount),
    m_Services {
        &m_LoggerService,
        &m_GraphicsLayer,
        &m_WorldState,
        &m_ModuleManager,
        &m_EventManager,
        &m_InputManager,
        &m_NetworkLayer,
        &m_TaskManager
    },
    m_Owner(owner),
    m_TopLevelLogger(m_LoggerService.CreateLogger("GameLoop")),
    m_EventWriter()
{
    for (const auto& system : owner->m_EventSystems)
    {
        m_EventManager.RegisterEventSystem(&system.second, 1);
    }
}

const Engine::Core::Runtime::ServiceTable *Engine::Core::Runtime::GameLoop::GameLoopController::GetServices() const 
{
    return &m_Services;
}

Engine::Core::Runtime::CallbackResult Engine::Core::Runtime::GameLoop::GameLoopController::Initialize() 
{
    CallbackResult graphicsInitResult = m_GraphicsLayer.InitializeSDL();
    if (graphicsInitResult.has_value())
        return graphicsInitResult;

    CallbackResult networkInitResult = m_NetworkLayer.Initialize();
    if (networkInitResult.has_value())
        return networkInitResult;

    return CallbackSuccess();
}

Engine::Core::Runtime::CallbackResult Engine::Core::Runtime::GameLoop::GameLoopController::LoadModules() 
{
    return m_ModuleManager.LoadModules(m_Owner->m_Modules, &m_Services);
}

Engine::Core::Runtime::CallbackResult Engine::Core::Runtime::GameLoop::GameLoopController::UnloadModules() 
{
    return m_ModuleManager.UnloadModules();
}

Engine::Core::Runtime::CallbackResult Engine::Core::Runtime::GameLoop::GameLoopController::BeginFrame() 
{
    // tick the timer
    m_WorldState.Tick();

    // input handling
    m_InputManager.ProcessSdlEvents();

    // begin update loop
    return m_GraphicsLayer.BeginFrame();
}

Engine::Core::Runtime::CallbackResult Engine::Core::Runtime::GameLoop::GameLoopController::Preupdate() 
{
    // pre-update
    for (InstancedSynchronousCallback callback :
        m_ModuleManager.m_PreupdateCallbacks) {
        auto result = callback.Callback(&m_Services, callback.InstanceState);
        if (result.has_value())
        return result;
    }

    return CallbackSuccess();
}

Engine::Core::Runtime::CallbackResult Engine::Core::Runtime::GameLoop::GameLoopController::EventUpdate() 
{
    // task-based event update
    while (m_EventManager.ExecuteAllSystems(&m_Services, m_EventWriter)) 
    {
        // mid-update events
        for (const InstancedSynchronousCallback &callback : m_ModuleManager.m_MidupdateCallbacks) 
        {
            callback.Callback(&m_Services, callback.InstanceState);
        }

        for (const InstancedEventCallback &routine : m_ModuleManager.m_EventCallbacks) 
        {
            Task task{TaskType::ProcessInputEvents};
            task.Payload.ProcessInputEventsTask = {routine, &m_EventWriter, 1};
            m_TaskManager.ScheduleWork(task);
        }

        size_t tasksLeft = m_ModuleManager.m_EventCallbacks.size();

        while (tasksLeft > 0) 
        {
            TaskResult nextResult = m_TaskManager.WaitOne();
            tasksLeft += nextResult.AdditionalTasks - 1;

            if (nextResult.Result.has_value())
                return nextResult.Result;
        }

        // post-update events
        for (const InstancedSynchronousCallback &callback : m_ModuleManager.m_PostupdateCallbacks) 
        {
            auto result = callback.Callback(&m_Services, callback.InstanceState);
            if (result.has_value())
                return result;
        }
    }

    return CallbackSuccess();
}

Engine::Core::Runtime::CallbackResult Engine::Core::Runtime::GameLoop::GameLoopController::RenderPass() 
{
    // render pass
    for (auto &callback : m_ModuleManager.m_RenderCallbacks) {
        CallbackResult callbackResult =
            callback.Callback(&m_Services, callback.InstanceState);
        if (callbackResult.has_value())
        return callbackResult;
    }

    return CallbackSuccess();
}

Engine::Core::Runtime::CallbackResult Engine::Core::Runtime::GameLoop::GameLoopController::EndFrame() 
{
    // last step in the update loop
    return m_GraphicsLayer.EndFrame();
}
