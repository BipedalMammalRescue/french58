#pragma once

#include "module_definition.h"
#include <vector>

namespace Engine {
namespace Core {
namespace Pipeline {

struct ModuleAssembly
{
    ModuleDefinition **Modules = nullptr;
    size_t ModuleCount = 0;
};

class EngineAssembly
{
  private:
    static std::vector<ModuleDefinition *> s_Modules;

  public:
    static int AddModule(ModuleDefinition *inModule)
    {
        s_Modules.push_back(inModule);
        return s_Modules.size();
    }
    static ModuleAssembly ListModules()
    {
        return {s_Modules.data(), s_Modules.size()};
    }
};

} // namespace Pipeline
} // namespace Core
} // namespace Engine
