#pragma once

#include "EngineCore/Containers/Uniform/sorted_array.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Runtime/service_table.h"
#include "RendererModule/Assets/mesh.h"
#include "RendererModule/Assets/material.h"
#include "RendererModule/Assets/render_pipeline.h"
#include "RendererModule/Components/directional_light.h"
#include "RendererModule/Components/mesh_renderer.h"

#include "EngineCore/Runtime/root_module.h"
#include "EngineCore/Pipeline/module_definition.h"
#include "EngineCore/Pipeline/hash_id.h"

#include "SDL3/SDL_gpu.h"

#include <unordered_map>
#include <vector>

namespace Engine::Core::Runtime {
struct RootModuleState;
}

namespace Engine::Extension::RendererModule {

class RendererModuleState 
{
public:
    const Core::Runtime::RootModuleState* RootModule;
    SDL_GPUBuffer* EmptyStorageBuffer;
    Core::Logging::Logger Logger;

    // shaders - ehh these are rarely used paths they can stay fragmented
    std::unordered_map<Core::Pipeline::HashId, SDL_GPUShader*> FragmentShaders;
    std::unordered_map<Core::Pipeline::HashId, SDL_GPUShader*> VertexShaders;

    // pipelines: treated as the hottest path so we keep them in the most uniform storage
    Core::Containers::Uniform::SortedArray<Assets::RenderPipeline, Assets::RenderPipelineComparer> PipelineIndex;

    // materials: also stored in a linear buffer but we build a more complicated index for it
    Core::Containers::Uniform::TrivialSortedArray<Core::Pipeline::HashId> LoadedMaterials;
    Core::Containers::Uniform::SortedArray<Assets::Material, Assets::MaterialComparer> MaterialIndex;

    // static meshes
    std::unordered_map<Core::Pipeline::HashId, RendererModule::Assets::StaticMesh> StaticMeshes;

    // mesh renderers
    Core::Containers::Uniform::SortedArray<Components::MeshRenderer, Components::MeshRendererComparer> MeshRenderers;

    // dynamic lighting (they are insanely expensive to update)
    std::vector<RendererModule::Components::DirectionalLight> DirectionalLights;
    SDL_GPUBuffer* DirectionalLightBuffer;

    RendererModuleState(Core::Runtime::ServiceTable* services);
};

Engine::Core::Pipeline::ModuleDefinition GetModuleDefinition();

} // namespace Engine::Extension::RendererModule