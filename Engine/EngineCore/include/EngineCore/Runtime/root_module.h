#pragma once

#include "EngineCore/Ecs/Components/camera_component.h"
#include "EngineCore/Ecs/Components/spatial_component.h"
#include "EngineCore/Pipeline/module_definition.h"
#include "EngineCore/Runtime/event_manager.h"

#include <optional>
#include <vector>

namespace Engine::Core::Runtime {

struct TransformUpdateEventData
{
    Ecs::Components::SpatialRelation NewTransform;
    int EntityId;
};

struct TickEventData
{
    float TotalTime = 0;
    float DeltaTime = 0;
};
// NOTE: object declaration needs to be in source file

// The root module authors data and manages states for part of the engine meant to be shared to other modules, but itself protected from them.
struct RootModuleState
{
    static Pipeline::ModuleDefinition GetDefinition();

    std::unordered_map<int, Ecs::Components::SpatialRelation> SpatialComponents;
    std::vector<Ecs::Components::Camera> CameraComponents;

    // output events
    std::optional<TickEventData> TickEvent;

    // input events
    EventOwner<TransformUpdateEventData> TransformUpdateEventOwner;
};

}