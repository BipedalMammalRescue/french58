#pragma once

#include "EngineCore/Pipeline/component_definition.h"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace Engine::Extension::RendererModule::Components {

bool CompileDirectionalLight(Core::Pipeline::RawComponent input, std::ostream* output);
Core::Runtime::CallbackResult LoadDirectionalLight(size_t count, Utils::Memory::MemStreamLite& stream, Core::Runtime::ServiceTable* services, void* moduleState);

struct DirectionalLight
{
    glm::vec3 Direction;
    float Padding0;
    glm::vec3 Color;
    float Padding1;
};

}