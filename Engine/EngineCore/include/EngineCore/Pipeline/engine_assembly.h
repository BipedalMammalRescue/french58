#pragma once

#include "module_definition.h"
#include <cstddef>

namespace Engine {
namespace Core {
namespace Pipeline {

struct ModuleAssembly
{
    ModuleDefinition *Modules = nullptr;
    size_t ModuleCount = 0;
};

// TODO: need a static table built into this class
class EngineAssembly
{
public:
    static ModuleAssembly ListModules();
};

} // namespace Pipeline
} // namespace Core
} // namespace Engine
