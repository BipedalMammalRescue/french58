#pragma once

#include "EngineCore/Pipeline/component_definition.h"
#include "EngineCore/Runtime/crash_dump.h"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>

namespace Engine::Core::Ecs::Components {

bool CompileSpatialComponent(Core::Pipeline::RawComponent input, std::ostream* output);
Runtime::CallbackResult LoadSpatialComponent(size_t count, Utils::Memory::MemStreamLite& stream, Core::Runtime::ServiceTable* services, void* moduleState);

struct SpatialRelation
{
    glm::vec3 Translation;
    glm::vec3 Scale;
    glm::quat Rotation;

    glm::mat4 Transform() const;
};

}