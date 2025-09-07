#include "EngineCore/Pipeline/engine_assembly.h"

#include <iostream>

using namespace Engine::Core;

int main()
{
    std::cout << "hello world" << std::endl;

    Pipeline::ModuleAssembly modules = Pipeline::EngineAssembly::ListModules();

    return 0;
}