#pragma once

#include "EngineCore/Ecs/Components/spatial_component.h"
#include "EngineCore/Pipeline/module_definition.h"

#include <vector>

namespace Engine::Core::Runtime {

// The root module authors data and manages states for part of the engine meant to be shared to other modules, but itself protected from them.
struct RootModuleState
{
    static Pipeline::ModuleDefinition GetDefinition();

    std::vector<Ecs::Components::SpatialRelation> SpatialComponents;
};

}