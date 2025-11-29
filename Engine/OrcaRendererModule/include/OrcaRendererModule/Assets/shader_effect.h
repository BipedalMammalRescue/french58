#pragma once

#include "EngineCore/Pipeline/hash_id.h"
#include "SDL3/SDL_gpu.h"
namespace Engine::Extension::OrcaRendererModule::Assets {

struct RenderGraphHeader;

enum class ShaderStage 
{
    Vertex,
    Fragment
};

struct ShaderResrouceBindingLocation
{
    ShaderStage Stage;
    uint32_t Slot;
};

enum class ResourceProviderType
{
    RenderGraph,
    EngineInjection,
    Material
};

struct ShaderResourceSource
{
    ResourceProviderType ProviderType;
    Core::Pipeline::HashId Name;
};

struct ShaderResourceBinding
{
    ShaderResrouceBindingLocation Location;
    ShaderResourceSource Source;
};

struct ShaderEffect
{
    // a loaded render graph never moves its relative location; this location does need to be runtime-generated tho
    // another note: we only allow 16 render graphs and 16 passes each so the chars actually aren't fully used
    // negative number means it's invalid
    RenderGraphHeader* RenderGraph;
    char RenderPassId;

    SDL_GPUGraphicsPipeline* Pipeline;

    // bindings (only allow 16 of them)
    size_t ResourceCount;
    ShaderResourceBinding Resources[16];
};

// when a shader is loaded into memory, we can pre-calculate their size and prepend this header
struct ShaderHeader
{
    size_t EffectCount;

    inline ShaderEffect* GetShaderEffects()
    {
        return (ShaderEffect*)(this + 1);
    }
};

}