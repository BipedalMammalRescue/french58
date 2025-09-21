#pragma once

#include "RendererModule/Assets/material.h"
#include "RendererModule/Assets/mesh.h"

#include "EngineCore/Pipeline/module_definition.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "RendererModule/Components/mesh_renderer.h"

#include <SDL3/SDL_gpu.h>
#include <unordered_map>

namespace Engine::Extension::RendererModule {

struct ModuleState 
{
    std::unordered_map<Core::Pipeline::HashId, SDL_GPUShader*> FragmentShaders;
    std::unordered_map<Core::Pipeline::HashId, SDL_GPUShader*> VertexShaders;
    std::unordered_map<Core::Pipeline::HashId, SDL_GPUGraphicsPipeline*> Materials;
    std::unordered_map<Core::Pipeline::HashId, RendererModule::Assets::GpuMesh> Meshes;

    std::vector<Components::MeshRenderer> MeshRendererComponents;
};

Engine::Core::Pipeline::ModuleDefinition GetModuleDefinition();

} // namespace Engine::Extension::RendererModule