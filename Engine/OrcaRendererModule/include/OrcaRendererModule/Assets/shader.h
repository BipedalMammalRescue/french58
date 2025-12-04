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
    Core::Pipeline::HashId RenderGraphName;
    char RenderPassId;

    // TODO: the shader effect is loaded directly from memory, therefore it can't really be laid out this way, we'll
    // need to separate them from the main shader effect data
    struct
    {
        bool Attempted;
        RenderGraphHeader *Graph;
    } RenderGraphCache;

    SDL_GPUGraphicsPipeline *Pipeline;

    // bindings (only allow 16 of them)
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
};

} // namespace Engine::Extension::OrcaRendererModule::Assets