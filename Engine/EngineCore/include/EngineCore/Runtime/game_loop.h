#pragma once

#include "EngineCore/Configuration/configuration_provider.h"
#include "EngineCore/Pipeline/asset_definition.h"
#include "EngineCore/Pipeline/component_definition.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/module_assembly.h"
#include "EngineCore/Runtime/world_state.h"

#include <unordered_map>

namespace Engine::Core::Runtime {

class GameLoop
{
private:
    // services
    Configuration::ConfigurationProvider m_ConfigurationProvider;
    Pipeline::ModuleAssembly m_Modules;
    std::unordered_map<Pipeline::HashIdTuple, Pipeline::ComponentDefinition> m_Components;
    std::unordered_map<Pipeline::HashIdTuple, Pipeline::AssetDefinition> m_Assets;

    // IO utilities (maybe move this to a service at sometime?)
    int LoadEntity(const char* filePath, ServiceTable services);

public:
    GameLoop(Pipeline::ModuleAssembly modules);
    int Run();
};

}