#pragma once

#include "component_pipeline.h"

namespace Engine {
namespace Core {
namespace Pipeline {

// includes everything defined in a module
struct ModuleDefinition
{
    // static definitions
    const char *Name = nullptr;
    ComponentPipeline *Components = nullptr;
    size_t ComponentCount = 0;
};

} // namespace Pipeline
} // namespace Core
} // namespace Engine
