#pragma once

#include "EngineCore/Ecs/Components/camera_component.h"
#include "EngineCore/Ecs/Components/spatial_component.h"
#include "EngineCore/Pipeline/module_definition.h"

#include <vector>

namespace Engine::Core::Runtime {

struct TransformUpdateEvent
{
    Ecs::Components::SpatialRelation NewTransform;
    int EntityId;
};

// The root module authors data and manages states for part of the engine meant to be shared to other modules, but itself protected from them.
struct RootModuleState
{
    static Pipeline::ModuleDefinition GetDefinition();

    std::unordered_map<int, Ecs::Components::SpatialRelation> SpatialComponents;
    std::vector<Ecs::Components::Camera> CameraComponents;

    std::vector<TransformUpdateEvent> TransformUpdateEvents;
};

}