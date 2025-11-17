#pragma once

#include "EngineCore/Pipeline/component_definition.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineUtils/Memory/memstream_lite.h"
#include "SDL3/SDL_gpu.h"

namespace Engine::Extension::RendererModule::Components {

bool CompileMeshRenderer(Core::Pipeline::RawComponent input, std::ostream* output);
Core::Runtime::CallbackResult LoadMeshRenderer(size_t count, Utils::Memory::MemStreamLite& stream, Core::Runtime::ServiceTable* services, void* moduleState);

struct MeshRenderer
{
    int Entity;
    Core::Pipeline::HashId Pipeline;
    Core::Pipeline::HashId Material;
    Core::Pipeline::HashId Mesh;

    SDL_GPUBuffer* VertexBuffer;
    SDL_GPUBuffer* IndexBuffer;
    uint32_t IndexCount;
};

// sort by pipeline id then by material id
struct MeshRendererComparer
{
    static int Compare(const MeshRenderer* a, const MeshRenderer* b)
    {
        if (a->Pipeline < b->Pipeline)
            return -1;
        if (a->Pipeline > b->Pipeline)
            return 1;

        if (a->Material < b->Material)
            return -1;
        if (a->Material > b->Material)
            return 1;
        return 0;
    }
};

}