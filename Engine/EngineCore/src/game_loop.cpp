#include "EngineCore/Runtime/game_loop.h"

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

#define IoCrashOut(error) Engine::Core::Runtime::Crash(__FILE__, __LINE__, )

using namespace Engine::Core::Runtime;

GameLoop::GameLoop(Pipeline::ModuleAssembly modules) : 
    m_ConfigurationProvider(),
    m_Modules(modules)
{
    // build component table
    for (size_t i = 0; i < modules.ModuleCount; i++)
    {
        Pipeline::ModuleDefinition module = modules.Modules[i];
        for (size_t j = 0; j < module.ComponentCount; j ++)
        {
            Pipeline::ComponentDefinition component = module.Components[j];
            Pipeline::HashIdTuple tuple { module.Name, component.Name };
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
            Pipeline::HashIdTuple tuple { module.Name, asset.Name };
            m_Assets[tuple] = asset;
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

CallbackResult GameLoop::RunCore(Pipeline::HashId initialEntityId)
{
    // initialize sdl
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
        std::string error("SDL initialization failed, error: ");
        error.append(SDL_GetError());
        return Crash(__FILE__, __LINE__, error);
	}

    // create logger
    Logging::LoggerService loggerService(m_ConfigurationProvider);
    CallbackResult loggerStartResult = loggerService.StartLogger();
    if (loggerStartResult.has_value())
        return loggerStartResult;

    // create other services
    GraphicsLayer graphicsLayer(&m_ConfigurationProvider, &loggerService);
    WorldState worldState(&m_ConfigurationProvider);
    ModuleManager moduleManager;
    EventManager eventManager(&loggerService);

    // insert systems
    for (auto pair : m_EventSystems)
    {
        eventManager.RegisterEventSystem(&pair.second, 1);
    }

    // initialize a local logger
    static const char* topLevelChannels[] = { "GameLoop" };
    Logging::Logger topLevelLogger = loggerService.CreateLogger(topLevelChannels, 1);

    // initialize services
    ServiceTable services {
        &loggerService,
        &graphicsLayer,
        &worldState,
        &moduleManager,
        &eventManager
    };

    CallbackResult serviceInitResult = graphicsLayer.InitializeSDL();
    if (serviceInitResult.has_value())
        return serviceInitResult;

    serviceInitResult = moduleManager.LoadModules(Pipeline::ListModules(), &services);
    if (serviceInitResult.has_value())
        return serviceInitResult;

    // load the first scene
    CallbackResult loadResult = LoadEntity(initialEntityId, services, &topLevelLogger);
    if (loadResult.has_value())
        return loadResult;

    TaskManager taskManager(&services, m_ConfigurationProvider.WorkerCount);

    EventWriter eventWriter;

    // game loop
    bool quit = false;
    SDL_Event e;
    while (!quit)
    {
        // tick the timer
        worldState.Tick();

        // Handle events on queue
        while (SDL_PollEvent(&e) != 0)
        {
            //User requests quit
            quit = e.type == SDL_EVENT_QUIT;
        }

        // begin update loop
        CallbackResult beginFrameResult = graphicsLayer.BeginFrame();
        if (beginFrameResult.has_value())
            return beginFrameResult;

        // task-based event update
        while (eventManager.ExecuteAllSystems(&services, eventWriter))
        {
            for (const InstancedEventCallback& routine : moduleManager.m_EventCallbacks)
            {
                Task task { TaskType::ProcessInputEvents };
                task.Payload.ProcessInputEventsTask = { routine, &eventWriter, 1 };
                taskManager.ScheduleWork(task);
            }

            size_t tasksLeft = moduleManager.m_EventCallbacks.size();

            while (tasksLeft > 0)
            {
                TaskResult nextResult = taskManager.WaitOne();
                tasksLeft += nextResult.AdditionalTasks - 1;

                if (nextResult.Result.has_value())
                    return nextResult.Result;
            }
        }

        // render pass
        for (auto& callback : moduleManager.m_RenderCallbacks)
        {
            CallbackResult callbackResult = callback.Callback(&services, callback.InstanceState);
            if (callbackResult.has_value())
                return callbackResult;
        }
        
        // last step in the update loop
        CallbackResult endFrameResult = graphicsLayer.EndFrame();
        if (endFrameResult.has_value())
            return endFrameResult;
    }

    topLevelLogger.Information("Game exiting without error.");
    return CallbackSuccess();
}

int GameLoop::Run(Pipeline::HashId initialEntityId) 
{
    // allow crash to persistent outside the game loop
    CallbackResult gameError = RunCore(initialEntityId);

    // quit SDL
    SDL_Quit();

    if (!gameError.has_value())
        return 0;

    printf("*** GAME CRASHED ***\n");
    printf("location: %s : %d\n", gameError->File.c_str(), gameError->Line);
    printf("crash dump: %s\n", gameError->ErrorDetail.c_str());
    return 1;
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

CallbackResult GameLoop::LoadEntity(Pipeline::HashId entityId, ServiceTable services, Logging::Logger* logger)
{
    logger->Information("Loading entity {id}", {entityId});

    char pathBuffer[] = "CD0ED230BD87479C61DB68677CAA9506.bse_entity";
    Utils::String::BinaryToHex(16, entityId.Hash.data(), pathBuffer);

    std::ifstream entityFile;
    entityFile.open(pathBuffer);

    if (!entityFile.is_open())
    {
        static const char errorMessage[] = "Entity file can't be opened.";
        logger->Fatal(errorMessage);
        return Crash(__FILE__, __LINE__, EntityLoadingError(entityId, errorMessage));
    }

    // read the asset section
    if (!CheckMagicWord(0xCCBBFFF1, &entityFile))
    {
        static const char errorMessage[] = "Entity magic word for asset section mismatch.";
        logger->Fatal(errorMessage);
        return Crash(__FILE__, __LINE__, EntityLoadingError(entityId, errorMessage));
    }

    int assetGroupCount = 0;
    entityFile.read((char*)&assetGroupCount, sizeof(int));
    for (int assetGroupIndex = 0; assetGroupIndex < assetGroupCount; assetGroupIndex ++)
    {
        Pipeline::HashIdTuple assetGroupId;
        entityFile.read((char*)&assetGroupId, 32);

        auto targetAssetType = m_Assets.find(assetGroupId);
        if (targetAssetType == m_Assets.end())
        {
            logger->Fatal("Module state not found for asset: {module}:{type}.", {assetGroupId.First, assetGroupId.Second});
            return Crash(__FILE__, __LINE__, EntityLoadingError(entityId, "Asset definition not found."));
        }

        auto targetModuleState = services.ModuleManager->m_LoadedModules.find(assetGroupId.First);
        if (targetModuleState == services.ModuleManager->m_LoadedModules.end())
        {
            logger->Fatal("Module state not found: {module}.", {assetGroupId.First});
            return Crash(__FILE__, __LINE__, EntityLoadingError(entityId, "Module state not found for asset."));
        }

        StreamAssetEnumerator enumerator(&entityFile);
        targetAssetType->second.Load(&enumerator, &services, targetModuleState->second.State);
    }

    // read the entities
    if (!CheckMagicWord(0xCCBBFFF2, &entityFile) || !services.WorldState->LoadEntities(&entityFile))
    {
        static const char errorMessage[] = "Entity magic word for entity section mismatch.";
        logger->Fatal(errorMessage);
        return Crash(__FILE__, __LINE__, EntityLoadingError(entityId, errorMessage));
    }

    // read the components
    if (!CheckMagicWord(0xCCBBFFF3, &entityFile))
    {
        static const char errorMessage[] = "Entity magic word for component section mismatch.";
        logger->Fatal(errorMessage);
        return Crash(__FILE__, __LINE__, EntityLoadingError(entityId, errorMessage));
    }

    int componentGroupCount = 0;
    entityFile.read((char*)&componentGroupCount, sizeof(int));
    for (int componentGroupIndex = 0; componentGroupIndex < componentGroupCount; componentGroupIndex ++)
    {
        Pipeline::HashIdTuple componentGroupId;
        entityFile.read((char*)&componentGroupId, 32);
        
        auto targetComponent = m_Components.find(componentGroupId);
        if (targetComponent == m_Components.end())
        {
            logger->Fatal("Module state not found for component: {module}:{type}.", {componentGroupId.First, componentGroupId.Second});
            return Crash(__FILE__, __LINE__, EntityLoadingError(entityId, "Asset definition not found."));
        }

        auto targetModuleState = services.ModuleManager->m_LoadedModules.find(componentGroupId.First);
        if (targetModuleState == services.ModuleManager->m_LoadedModules.end())
        {
            logger->Fatal("Module state not found: {module}.", {componentGroupId.First});
            return Crash(__FILE__, __LINE__, EntityLoadingError(entityId, "Module state not found for asset."));
        }

        int componentCount = 0;
        entityFile.read((char*)&componentCount, sizeof(int));
        targetComponent->second.Load(componentCount, &entityFile, &services, targetModuleState->second.State);
    }

    logger->Verbose("Loaded {assetC} asset groups, {componentCount} component groups.", {assetGroupCount, componentGroupCount});
    return CallbackSuccess();
}
