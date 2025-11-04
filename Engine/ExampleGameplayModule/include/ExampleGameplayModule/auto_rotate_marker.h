#pragma once

#include "EngineCore/Pipeline/component_definition.h"

namespace Engine::Extension::ExampleGameplayModule {

bool CompileMarker(Core::Pipeline::RawComponent input, std::ostream* output);
Core::Runtime::CallbackResult LoadMarker(size_t count, std::istream* input, Core::Runtime::ServiceTable* services, void* moduleState);

}