#include "Pipeline/engine_assembly.h"
#include <iostream>

int main()
{
    std::cout << "hello world" << std::endl;

    // TODO: get and list engine assembly components
    Engine::Core::Pipeline::ModuleAssembly modules = Engine::Core::Pipeline::EngineAssembly::ListModules();

    for (size_t i = 0; i < modules.ModuleCount; i++)
    {
        std::cout << modules.Modules[i]->Name << std::endl;
    }
}
