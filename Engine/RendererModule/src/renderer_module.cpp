#include "RendererModule/renderer_module.h"
#include "RendererModule/Assets/mesh.h"

#include "EngineCore/Pipeline/asset_pipeline.h"
#include "EngineCore/Pipeline/module_definition.h"

using namespace Engine;

Engine::Core::Pipeline::ModuleDefinition Engine::Extension::RendererModule::GetModuleDefinition()
{
    static Core::Pipeline::AssetPipeline allAssets[] = { 
        {
        Extension::RendererModule::Assets::Mesh::GetDefinition(),
        Extension::RendererModule::Assets::Mesh::Build
        }
    };

    static Core::Pipeline::ModuleDefinition rendererModule = {
        "RendererModule",
        {
            allAssets,
            1
        }
    };

    return rendererModule;
}
