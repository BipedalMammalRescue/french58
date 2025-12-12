#pragma once

#include "EngineCore/AssetManagement/asset_loading_context.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/service_table.h"
#include "SDL3/SDL_gpu.h"

namespace Engine::Extension::OrcaRendererModule::Assets {

// TODO: this "mesh" or maybe "geometry" is a renderer concept (as opposed to graph, pass or
// material which are all seen as generic resource collections by the renderer), need to update the
// renderer design to address this problem
struct Mesh
{
    size_t IndexCount;
    SDL_GPUBuffer *VertexBuffer;
    SDL_GPUBuffer *IndexBuffer;
};

Engine::Core::Runtime::CallbackResult ContextualizeMesh(
    Engine::Core::Runtime::ServiceTable *services, void *moduleState,
    Engine::Core::AssetManagement::AssetLoadingContext *outContext, size_t contextCount);

Engine::Core::Runtime::CallbackResult IndexMesh(
    Engine::Core::Runtime::ServiceTable *services, void *moduleState,
    Engine::Core::AssetManagement::AssetLoadingContext *inContext);

} // namespace Engine::Extension::OrcaRendererModule::Assets