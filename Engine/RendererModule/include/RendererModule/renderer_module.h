#pragma once

#include "RendererModule/Assets/mesh.h"
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

struct ModuleState 
{
    const Core::Runtime::RootModuleState* RootModule;

    std::unordered_map<Core::Pipeline::HashId, SDL_GPUShader*> FragmentShaders;
    std::unordered_map<Core::Pipeline::HashId, SDL_GPUShader*> VertexShaders;
    std::unordered_map<Core::Pipeline::HashId, SDL_GPUGraphicsPipeline*> Materials;
    std::unordered_map<Core::Pipeline::HashId, RendererModule::Assets::GpuMesh> Meshes;
    
    std::vector<Components::MeshRenderer> MeshRendererComponents;

    std::vector<RendererModule::Components::DirectionalLight> DirectionalLights;
    SDL_GPUBuffer* DirectionalLightBuffer;
};

Engine::Core::Pipeline::ModuleDefinition GetModuleDefinition();

} // namespace Engine::Extension::RendererModule