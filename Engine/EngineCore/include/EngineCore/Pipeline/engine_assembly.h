#pragma once

#include "module_definition.h"

#include <cstddef>

namespace Engine::Core::Pipeline {

struct ModuleAssembly {
  ModuleDefinition *Modules = nullptr;
  size_t ModuleCount = 0;
};

ModuleAssembly ListModules();

} // namespace Engine::Core::Pipeline
