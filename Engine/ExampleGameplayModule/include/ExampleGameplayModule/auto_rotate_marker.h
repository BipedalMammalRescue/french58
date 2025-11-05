#pragma once

#include "EngineCore/Pipeline/component_definition.h"
#include "EngineCore/Pipeline/hash_id.h"

namespace Engine::Extension::ExampleGameplayModule {

bool CompileMarker(Core::Pipeline::RawComponent input, std::ostream* output);
Core::Runtime::CallbackResult LoadMarker(size_t count, std::istream* input, Core::Runtime::ServiceTable* services, void* moduleState);

struct RotationMarker
{
    int Entity;
    Core::Pipeline::HashId InputMethod;
};

}