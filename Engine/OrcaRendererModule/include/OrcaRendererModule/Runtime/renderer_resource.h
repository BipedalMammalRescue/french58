#pragma once

#include "EngineCore/Pipeline/hash_id.h"
#include "SDL3/SDL_gpu.h"

namespace Engine::Extension::OrcaRendererModule::Runtime {

// Opaque pointer used only to identify a resource owner
struct ResourceOwner;

enum class ResourceType
{
    Invalid,
    Texture,
    StorageBuffer,
    UniformBuffer
};

struct RendererResource
{
    ResourceType Type;
    union {
        SDL_GPUTexture *Texture;
        SDL_GPUBuffer *StorageBuffer;
        struct
        {
            void *Data;
            size_t Length;
        } Uniform;
    };
};

struct NamedRendererResource
{
    Core::Pipeline::HashId InterfaceName;
    uint32_t ResourceId;
};

} // namespace Engine::Extension::OrcaRendererModule::Runtime