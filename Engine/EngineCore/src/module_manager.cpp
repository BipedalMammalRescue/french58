#include "EngineCore/Runtime/module_manager.h"
#include "EngineCore/Pipeline/engine_callback.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/root_module.h"
#include "EngineUtils/String/hex_strings.h"
#include <string>

using namespace Engine::Core::Runtime;

static const char* LogChannels[] = { "ModuleManager" };

static std::string ModuleLoadingError(const char* reason, Engine::Core::Pipeline::HashId moduleName)
{
    std::string message = "Failed to load module ";
    
    char nameBuffer[33] = {0};
    Engine::Utils::String::BinaryToHex(16, moduleName.Hash.data(), nameBuffer);
    message.append(nameBuffer);

    message.append(" becuase ");
    message.append(reason);
    return message;
}

CallbackResult ModuleManager::LoadModules(const Pipeline::ModuleAssembly& modules, ServiceTable* services)
{
    m_Services = services;
    m_Logger = services->LoggerService->CreateLogger(LogChannels, 1);

    // load modules
    for (size_t i = 0; i < modules.ModuleCount; i++)
    {
        Pipeline::ModuleDefinition moduleDef = modules.Modules[i];
        void* newState = moduleDef.Initialize(services);

        auto foundCollision = m_LoadedModules.find(moduleDef.Name);
        if (foundCollision != m_LoadedModules.end())
            return Crash(__FILE__, __LINE__, ModuleLoadingError("module name collision", moduleDef.Name));

        m_LoadedModules[moduleDef.Name] = {moduleDef, newState};

        // set up callback table
        for (size_t j = 0; j < moduleDef.SynchronousCallbackCount; j++) 
        {
            Pipeline::SynchronousCallback callback = moduleDef.SynchronousCallbacks[j];
            switch (callback.Stage)
            {
                case Pipeline::EngineCallbackStage::Preupdate:
                    m_PreupdateCallbacks.push_back({ callback.Callback, newState });
                    break;
                case Pipeline::EngineCallbackStage::Render:
                    m_RenderCallbacks.push_back({ callback.Callback, newState });
                    break;
            }
        }

        for (size_t j = 0; j < moduleDef.EventCallbackCount; j++) 
        {
            Pipeline::EventCallback callback = moduleDef.EventCallbacks[j];
            m_EventCallbacks.push_back({ callback.Callback, newState });
        }

        m_Logger.Information("Loaded module {moduleId}", { moduleDef.Name });
    }

    return CallbackSuccess();
}

ModuleManager::~ModuleManager()
{
    for (auto module : m_LoadedModules)
    {
        module.second.Definition.Dispose(m_Services, module.second.State);
        m_Logger.Information("Unloaded module {moduleId}", { module.first });
    }
}

const void* ModuleManager::FindModule(const Pipeline::HashId& name) const
{
    auto foundModule = m_LoadedModules.find(name);
    if (foundModule == m_LoadedModules.end())
        return nullptr;
    
    return foundModule->second.State;
}

const void* ModuleManager::FindModule(const Pipeline::HashId&& name) const
{
    auto foundModule = m_LoadedModules.find(name);
    if (foundModule == m_LoadedModules.end())
        return nullptr;
    
    return foundModule->second.State;
}

const RootModuleState* ModuleManager::GetRootModule() const
{
    return static_cast<const RootModuleState*>(FindModule(RootModuleState::GetDefinition().Name));
}