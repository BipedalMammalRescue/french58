#pragma once

#include "EngineCore/Pipeline/fwd.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/fwd.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "SDL3/SDL_gpu.h"

namespace Engine::Extension::RendererModule::Assets {

struct GpuMesh
{
    SDL_GPUBuffer* IndexBuffer;
    unsigned int IndexCount;
    SDL_GPUBuffer* VertexBuffer;
};

Core::Runtime::CallbackResult LoadMesh(Core::Pipeline::IAssetEnumerator *inputStreams,
              Core::Runtime::ServiceTable *services,
              void *moduleState);

Core::Runtime::CallbackResult UnloadMesh(Core::Pipeline::HashId *ids, size_t count,
                Core::Runtime::ServiceTable *services, void *moduleState);

void DisposeMesh(Core::Runtime::ServiceTable *services, GpuMesh mesh);
}

