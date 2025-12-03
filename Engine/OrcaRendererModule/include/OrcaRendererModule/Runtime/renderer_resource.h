#pragma once

#include "EngineCore/Pipeline/hash_id.h"
#include "SDL3/SDL_gpu.h"

namespace Engine::Extension::OrcaRendererModule::Runtime {

// Opaque pointer used only to identify a resource owner
struct ResourceOwner;

enum class ResourceType
{
    Texture,
    StorageBuffer,
    UniformBuffer
};

struct RendererResource
{
    Core::Pipeline::HashId Name;
    ResourceType Type;
    union {
        SDL_GPUTexture* Texture;
        SDL_GPUBuffer* StorageBuffer;
        struct {
            void* Data;
            size_t Length;
        } UniformProvider;
    };
};

struct RendererResourceCollection
{
    RendererResource* Resources;
    size_t Count;

    RendererResource* Find(Core::Pipeline::HashId name);
};

}