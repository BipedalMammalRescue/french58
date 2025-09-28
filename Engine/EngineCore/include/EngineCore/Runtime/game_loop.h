#pragma once

#include "EngineCore/Configuration/configuration_provider.h"
#include "EngineCore/Pipeline/asset_definition.h"
#include "EngineCore/Pipeline/component_definition.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/module_assembly.h"
#include "EngineCore/Runtime/world_state.h"
#include "EngineCore/Pipeline/hash_id.h"

#include <unordered_map>

namespace Engine::Core::Logging {
class Logger;
}

namespace Engine::Core::Runtime {

enum class FileIoResult
{
    Success,
    NotOpened,
    Corrupted,
    AssetGroupNotFound,
    ModuleNotFound,
    ComponentGroupNotFound
};

class GameLoop
{
private:
    // services
    Configuration::ConfigurationProvider m_ConfigurationProvider;
    Pipeline::ModuleAssembly m_Modules;
    std::unordered_map<Pipeline::HashIdTuple, Pipeline::ComponentDefinition> m_Components;
    std::unordered_map<Pipeline::HashIdTuple, Pipeline::AssetDefinition> m_Assets;

    // IO utilities (maybe move this to a service at sometime?)
    FileIoResult LoadEntity(Pipeline::HashId entityId, ServiceTable services, Logging::Logger* logger);

    // crash handling
    void RaiseIoError(FileIoResult result, const char* decorator) const;
    void RaiseSdlError() const;

public:
    GameLoop(Pipeline::ModuleAssembly modules);
    int Run(Pipeline::HashId initialEntity);
};

}