#pragma once

#include "EngineCore/Pipeline/module_definition.h"

#include <cstddef>

namespace Engine::Core::Pipeline {

struct ModuleAssembly 
{
    const ModuleDefinition *Modules = nullptr;
    size_t ModuleCount = 0;
};

ModuleAssembly ListModules();

} // namespace Engine::Core::Pipeline
