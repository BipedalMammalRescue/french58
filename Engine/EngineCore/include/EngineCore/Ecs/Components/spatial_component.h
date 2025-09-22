#pragma once

#include "EngineCore/Pipeline/component_definition.h"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>

namespace Engine::Core::Ecs::Components {

bool CompileSpatialComponent(Core::Pipeline::RawComponent input, std::ostream* output);
void LoadSpatialComponent(size_t count, std::istream* input, Core::Runtime::ServiceTable* services, void* moduleState);

struct SpatialRelation
{
    glm::vec3 Translation;
    glm::vec3 Scale;
    glm::quat Rotation;

    glm::mat4 Transform();
};

}