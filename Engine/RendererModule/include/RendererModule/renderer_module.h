#pragma once

#include "RendererModule/Assets/mesh.h"
#include "RendererModule/Assets/material.h"
#include "RendererModule/Assets/render_pipeline.h"
#include "RendererModule/Components/directional_light.h"
#include "RendererModule/Components/mesh_renderer.h"

#include <EngineCore/Runtime/root_module.h>
#include <EngineCore/Pipeline/module_definition.h>
#include <EngineCore/Pipeline/hash_id.h>

#include <SDL3/SDL_gpu.h>
#include <unordered_map>

namespace Engine::Core::Runtime {
struct RootModuleState;
}

namespace Engine::Extension::RendererModule {

struct IndexedMaterial
{
    Assets::Material Material;
    std::vector<Components::MeshRenderer> Meshes;
};

struct IndexedPipeline
{
    Assets::RenderPipeline Pipeline;
    std::vector<IndexedMaterial> Materials;
};

struct MaterialLocation
{
    size_t Pipeline;
    size_t Material;
};

struct ModuleState 
{
    const Core::Runtime::RootModuleState* RootModule;
    SDL_GPUBuffer* EmptyStorageBuffer;

    std::unordered_map<Core::Pipeline::HashId, SDL_GPUShader*> FragmentShaders;
    std::unordered_map<Core::Pipeline::HashId, SDL_GPUShader*> VertexShaders;
    std::unordered_map<Core::Pipeline::HashId, RendererModule::Assets::GpuMesh> Meshes;

    std::unordered_map<Core::Pipeline::HashId, size_t> PipelineIndex;
    std::unordered_map<Core::Pipeline::HashId, MaterialLocation> MaterialIndex;
    std::vector<IndexedPipeline> Pipelines;

    // TODO: allow storage buffers to be configured
    std::vector<Assets::InjectedUniform> InjectedUniforms;
    std::vector<Assets::InjectedStorageBuffer> InjectedStorageBuffers;
    std::vector<Assets::ConfiguredUniform> ConfiguredUniforms;

    std::vector<RendererModule::Components::DirectionalLight> DirectionalLights;
    SDL_GPUBuffer* DirectionalLightBuffer;
};

Engine::Core::Pipeline::ModuleDefinition GetModuleDefinition();

} // namespace Engine::Extension::RendererModule