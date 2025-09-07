#include "RendererModule/renderer_module.h"
#include "RendererModule/Assets/shader.h"
#include "RendererModule/Assets/material.h"
#include "RendererModule/Assets/mesh.h"

#include "EngineCore/Pipeline/asset_pipeline.h"
#include "EngineCore/Pipeline/module_definition.h"
#include "RendererModule/Assets/shader.h"

using namespace Engine;
using namespace Engine::Extension::RendererModule;

Engine::Core::Pipeline::ModuleDefinition Engine::Extension::RendererModule::GetModuleDefinition()
{
    
    static Core::Pipeline::AssetPipeline allAssets[] = {
        {
        Assets::Mesh::GetDefinition(),
        Assets::Mesh::Build
        },
        {
            Assets::FragmentShader::GetDefinition(),
            Assets::FragmentShader::Build
        },
        {
            Assets::VertexShader::GetDefinition(),
            Assets::VertexShader::Build
        },
        {
            Assets::Material::GetDefinition(),
            Assets::Material::Build
        },
    };

    constexpr size_t AssetCount = sizeof(allAssets) / sizeof(Core::Pipeline::AssetPipeline);

    static Core::Pipeline::ModuleDefinition rendererModule = {
        "RendererModule",
        {
            allAssets,
            AssetCount
        }
    };

    return rendererModule;
}
