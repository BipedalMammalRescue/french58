#include "EngineCore/Runtime/module_manager.h"
#include "EngineCore/Logging/logger_service.h"
#include "EngineCore/Pipeline/engine_callback.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/root_module.h"
#include "EngineCore/Runtime/service_table.h"
#include "EngineUtils/String/hex_strings.h"
#include <string>

using namespace Engine::Core::Runtime;

static const char *LogChannels[] = {"ModuleManager"};

static std::string ModuleLoadingError(const char *reason, Engine::Core::Pipeline::HashId moduleName)
{
    std::string message = "Failed to load module ";

    char nameBuffer[33] = {0};
    Engine::Utils::String::BinaryToHex(16, moduleName.Hash.data(), nameBuffer);
    message.append(nameBuffer);

    message.append(" becuase ");
    message.append(reason);
    return message;
}

ModuleManager::ModuleManager(Engine::Core::Logging::LoggerService *loggerService)
    : m_Logger(loggerService->CreateLogger("ModuleManager"))
{
}

CallbackResult ModuleManager::LoadModules(const Pipeline::ModuleAssembly &modules,
                                          ServiceTable *services)
{
    m_Services = services;

    // load modules
    for (size_t i = 0; i < modules.ModuleCount; i++)
    {
        Pipeline::ModuleDefinition moduleDef = modules.Modules[i];
        void *newState = moduleDef.Initialize(services);

        auto foundCollision = m_LoadedModules.find(moduleDef.Name.Hash);
        if (foundCollision != m_LoadedModules.end())
            return Crash(__FILE__, __LINE__,
                         ModuleLoadingError("module name collision", moduleDef.Name.Hash));

        m_LoadedModules[moduleDef.Name.Hash] = {moduleDef, newState};

        // set up callback table
        for (size_t j = 0; j < moduleDef.SynchronousCallbackCount; j++)
        {
            Pipeline::SynchronousCallback callback = moduleDef.SynchronousCallbacks[j];
            switch (callback.Stage)
            {
            case Pipeline::SynchronousCallbackStage::Preupdate:
                m_PreupdateCallbacks.push_back({callback.Callback, newState});
                break;
            case Pipeline::SynchronousCallbackStage::Render:
                m_RenderCallbacks.push_back({callback.Callback, newState});
                break;
            case Pipeline::SynchronousCallbackStage::MidUpdate:
                m_MidupdateCallbacks.push_back({callback.Callback, newState});
                break;
            case Pipeline::SynchronousCallbackStage::PostUpdate:
                m_PostupdateCallbacks.push_back({callback.Callback, newState});
                break;
            }
        }

        for (size_t j = 0; j < moduleDef.EventCallbackCount; j++)
        {
            Pipeline::EventCallback callback = moduleDef.EventCallbacks[j];
            m_EventCallbacks.push_back({callback.Callback, newState});

            // render state
            for (size_t rendererPluginIndex = 0;
                 rendererPluginIndex < moduleDef.RendererPluginCount; rendererPluginIndex++)
            {
                auto pluginDef = moduleDef.RendererPlugins[rendererPluginIndex];
                void *pluginState = pluginDef.Initialize(services, newState);

                m_RendererPlugins.push_back({
                    .Definition = pluginDef,
                    .PluginState = pluginState,
                    .ModuleState = newState,
                });
            }
        }

        m_Logger.Information("Loaded module {}:{}", moduleDef.Name.DisplayName,
                             moduleDef.Name.Hash);
    }

    return CallbackSuccess();
}

CallbackResult ModuleManager::UnloadModules()
{
    for (auto module : m_LoadedModules)
    {
        module.second.Definition.Dispose(m_Services, module.second.State);
        m_Logger.Information("Unloaded module {}:{}", module.second.Definition.Name.DisplayName,
                             module.second.Definition.Name.Hash);
    }

    m_LoadedModules.clear();
    return CallbackSuccess();
}

ModuleManager::~ModuleManager()
{
    UnloadModules();
}

void *ModuleManager::FindModuleMutable(const Pipeline::HashId &name)
{
    auto foundModule = m_LoadedModules.find(name);
    if (foundModule == m_LoadedModules.end())
        return nullptr;

    return foundModule->second.State;
}

const void *ModuleManager::FindModule(const Pipeline::HashId &name) const
{
    auto foundModule = m_LoadedModules.find(name);
    if (foundModule == m_LoadedModules.end())
        return nullptr;

    return foundModule->second.State;
}

const void *ModuleManager::FindModule(const Pipeline::HashId &&name) const
{
    auto foundModule = m_LoadedModules.find(name);
    if (foundModule == m_LoadedModules.end())
        return nullptr;

    return foundModule->second.State;
}

const RootModuleState *ModuleManager::GetRootModule() const
{
    return static_cast<const RootModuleState *>(
        FindModule(RootModuleState::GetDefinition().Name.Hash));
}