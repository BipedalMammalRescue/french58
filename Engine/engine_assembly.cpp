#include "RendererModule/renderer_module.h"

#include <EngineCore/Pipeline/module_definition.h>
#include <EngineCore/Pipeline/engine_assembly.h>

using namespace Engine;

// define modules here
Core::Pipeline::ModuleDefinition Modules[] = {
    Extension::RendererModule::GetModuleDefinition()
};

constexpr size_t ModuleCount = sizeof(Modules) / sizeof(Core::Pipeline::ModuleDefinition);

Core::Pipeline::ModuleAssembly Core::Pipeline::EngineAssembly::ListModules() 
{
    return { Modules, ModuleCount };
}