#pragma once

#include "EngineCore/AssetManagement/asset_loading_context.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/fwd.h"
#include "SDL3/SDL_gpu.h"

namespace Engine::Extension::RendererModule::Assets {

struct StaticMesh
{
    SDL_GPUBuffer* IndexBuffer;
    unsigned int IndexCount;
    SDL_GPUBuffer* VertexBuffer;
};

Core::Runtime::CallbackResult ContextualizeStaticMesh(Core::Runtime::ServiceTable *services, void *moduleState, Core::AssetManagement::AssetLoadingContext* outContext, size_t contextCount);
Core::Runtime::CallbackResult IndexStaticMesh(Core::Runtime::ServiceTable *services, void *moduleState, Core::AssetManagement::AssetLoadingContext* inContext);
}

