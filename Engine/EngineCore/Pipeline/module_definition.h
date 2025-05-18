#pragma once

#include "component_pipeline.h"

namespace Engine::Core::Pipeline {

// includes everything defined in a module
struct ModuleDefinition
{
    // static definitions
    const char *Name = nullptr;
    ComponentPipeline *Components = nullptr;
    size_t ComponentCount = 0;
};

} // namespace Engine::Core::Pipeline