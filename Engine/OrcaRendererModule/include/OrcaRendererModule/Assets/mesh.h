#pragma once

#include "EngineCore/AssetManagement/asset_loading_context.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/service_table.h"
#include "SDL3/SDL_gpu.h"

namespace Engine::Extension::OrcaRendererModule::Assets {

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