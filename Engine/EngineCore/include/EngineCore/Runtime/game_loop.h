#pragma once

#include "EngineCore/Configuration/configuration_provider.h"
#include "EngineCore/Pipeline/module_assembly.h"
#include "EngineCore/Runtime/graphics_layer.h"
#include "EngineCore/Runtime/module_manager.h"
#include "EngineCore/Runtime/world_state.h"

namespace Engine::Core::Runtime {

class GameLoop
{
private:
    // services
    Configuration::ConfigurationProvider m_ConfigurationProvider;
    GraphicsLayer m_GraphicsLayer;
    WorldState m_WorldState;
    ModuleManager m_ModuleManager;
    Pipeline::ModuleAssembly m_Modules;

public:
    GameLoop(Pipeline::ModuleAssembly modules);
    int Run();
};

}