#include "EngineCore/Runtime/root_module.h"
#include "EngineCore/Ecs/Components/spatial_component.h"
#include "EngineCore/Pipeline/component_definition.h"
#include "EngineCore/Pipeline/module_definition.h"

using namespace Engine::Core;
using namespace Engine::Core::Runtime;

static void* InitializeRootModule(ServiceTable* services)
{
    return new RootModuleState();
}

static void DisposeRootModule(ServiceTable* services, void* moduleState) 
{
    delete static_cast<RootModuleState*>(moduleState);
}

Pipeline::ModuleDefinition RootModuleState::GetDefinition() 
{
    static const Pipeline::ComponentDefinition rootComponents[] {
        {
            md5::compute("SpatialComponent"),
            Ecs::Components::CompileSpatialComponent,
            Ecs::Components::LoadSpatialComponent
        }
    };

    return Pipeline::ModuleDefinition {
        md5::compute("EngineRootModule"),
        InitializeRootModule,
        DisposeRootModule,
        nullptr,
        0,
        nullptr,
        0,
        rootComponents,
        sizeof(rootComponents) / sizeof(Pipeline::ComponentDefinition)
    };
}