#pragma once

#include "module_definition.h"

namespace Engine {
namespace Core {
namespace Pipeline {

struct ModuleAssembly
{
    ModuleDefinition *Modules = nullptr;
    size_t ModuleCount = 0;
};

ModuleAssembly ListModules();

} // namespace Pipeline
} // namespace Core
} // namespace Engine
