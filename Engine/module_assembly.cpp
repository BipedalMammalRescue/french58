#include "InputModule/input_module.h"
#include "RendererModule/renderer_module.h"
#include "ExampleGameplayModule/example_gameplay_module.h"

#include <EngineCore/Runtime/root_module.h>
#include <EngineCore/Pipeline/module_definition.h>
#include <EngineCore/Pipeline/module_assembly.h>

using namespace Engine;

// TODO: this file need to be generated based on a list of modules during installs

// define modules here
static const Core::Pipeline::ModuleDefinition Modules[] = {
    Engine::Core::Runtime::RootModuleState::GetDefinition(),
    Extension::InputModule::GetModuleDefinition(),
    Extension::RendererModule::GetModuleDefinition(),
    Extension::ExampleGameplayModule::GetDefinition()
};

constexpr size_t ModuleCount = sizeof(Modules) / sizeof(Core::Pipeline::ModuleDefinition);

Core::Pipeline::ModuleAssembly Core::Pipeline::ListModules() 
{
    return { Modules, ModuleCount };  
}