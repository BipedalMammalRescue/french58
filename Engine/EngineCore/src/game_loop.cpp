#include "EngineCore/Runtime/game_loop.h"

#include "EngineCore/Configuration/configuration_provider.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Logging/logger_service.h"
#include "EngineCore/Pipeline/engine_callback.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/module_assembly.h"
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
#include <md5.h>
#include <string>

using namespace Engine::Core::Runtime;

GameLoop::GameLoop(Pipeline::ModuleAssembly modules, const Configuration::ConfigurationProvider& configs) : 
    m_ConfigurationProvider(configs),
    m_Modules(modules)
{
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
    GameLoopController controller(m_Modules, m_ConfigurationProvider, this);

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
    GameLoopController controller(m_Modules, m_ConfigurationProvider, this);

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
        gameresult = controller.PollAsyncIoEvents();
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

        // enter rendering critical path
        gameresult = controller.BeginFrame();
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

static const char* TopLevelLogChannels[] = { "GameLoop" };

GameLoop::GameLoopController::GameLoopController(Engine::Core::Pipeline::ModuleAssembly modules, Engine::Core::Configuration::ConfigurationProvider configs, GameLoop* owner)
    : m_LoggerService(configs),
    m_GraphicsLayer(&configs, &m_LoggerService),
    m_WorldState(&configs),
    m_ModuleManager(&m_LoggerService),
    m_EventManager(&m_LoggerService),
    m_InputManager(),
    m_NetworkLayer(&m_LoggerService),
    m_TaskManager(&m_Services, &m_LoggerService, configs.WorkerCount),
    m_TransientAllocator(&m_LoggerService),
    m_AssetManager(modules, &m_LoggerService, &m_Services),
    m_ContainerFactory(&m_LoggerService),
    m_Services {
        &m_LoggerService,
        &m_GraphicsLayer,
        &m_WorldState,
        &m_ModuleManager,
        &m_EventManager,
        &m_InputManager,
        &m_NetworkLayer,
        &m_TaskManager,
        &m_TransientAllocator,
        &m_AssetManager,
        &m_ContainerFactory
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
    // begin update loop
    return m_GraphicsLayer.BeginFrame();
}

Engine::Core::Runtime::CallbackResult Engine::Core::Runtime::GameLoop::GameLoopController::Preupdate() 
{
    // tick the timer
    m_WorldState.Tick();

    // input handling
    m_InputManager.ProcessSdlEvents();

    // pre-update
    for (InstancedSynchronousCallback callback : m_ModuleManager.m_PreupdateCallbacks) 
    {
        CallbackResult result = callback.Callback(&m_Services, callback.InstanceState);
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

Engine::Core::Runtime::CallbackResult Engine::Core::Runtime::GameLoop::GameLoopController::LoadEntity(Pipeline::HashId entityId)
{
    m_AssetManager.QueueEntity(entityId);
    return CallbackSuccess();
}

Engine::Core::Runtime::CallbackResult Engine::Core::Runtime::GameLoop::GameLoopController::PollAsyncIoEvents() 
{
    m_AssetManager.PollEvents();
    return CallbackSuccess();
}
