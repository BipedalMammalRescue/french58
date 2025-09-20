#pragma once

#include "RendererModule/Assets/mesh.h"

#include "EngineCore/Pipeline/module_definition.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Runtime/service_table.h"

#include <SDL3/SDL_gpu.h>
#include <unordered_map>

namespace Engine::Extension::RendererModule {

struct ModuleState 
{
    std::unordered_map<Core::Pipeline::HashId, SDL_GPUShader*> FragmentShaders;
    std::unordered_map<Core::Pipeline::HashId, SDL_GPUShader*> VertexShaders;
    std::unordered_map<Core::Pipeline::HashId, SDL_GPUGraphicsPipeline*> GraphicsPipelines;
    std::unordered_map<Core::Pipeline::HashId, RendererModule::Assets::GpuMesh> Meshes;
};

Engine::Core::Pipeline::ModuleDefinition GetModuleDefinition();

void* InitRendererModule(Core::Runtime::ServiceTable* services);
void DisposeRendererModule(Core::Runtime::ServiceTable* services, void* moduleState);

} // namespace Engine::Extension::RendererModule