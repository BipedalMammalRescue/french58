#pragma once

#include "EngineCore/Pipeline/component_definition.h"

namespace Engine::Core::Ecs::Components {

bool CompileCameraComponent(Pipeline::RawComponent input, std::ostream* output);
void LoadCameraComponent(size_t count, std::istream* input, Runtime::ServiceTable* services, void* moduleState);

// Supposedly it's helpful to treat the position of a camera as a basic concept owned by the core engine
struct Camera
{
    int Entity;
    bool IsPrimary;
};

}