#pragma once

#include "EngineCore/Pipeline/hash_id.h"
#include "SDL3/SDL_gpu.h"

namespace Engine::Extension::OrcaRendererModule::Assets {

struct RenderGraph;

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
    Material,
    Object
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

// All the information used to define a single pipeline state object and how to use it.
// This data structure is serialized directly to disk so don't put runtime-dependent data here.
struct ShaderEffect
{
    Core::Pipeline::HashId RenderGraphName;
    char RenderPassId;

    SDL_GPUGraphicsPipeline *Pipeline;

    // bindings (only allow 16 of them) (make sure resources are sorted by name)
    size_t ResourceCount;
    ShaderResourceBinding Resources[16];
};

struct Shader
{
    size_t EffectCount;

    inline ShaderEffect *GetShaderEffects()
    {
        return (ShaderEffect *)(this + 1);
    }

    inline size_t *GetRenderGraphReferences()
    {
        return (size_t *)(GetShaderEffects() + EffectCount);
    }
};

} // namespace Engine::Extension::OrcaRendererModule::Assets