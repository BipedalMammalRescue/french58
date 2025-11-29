#pragma once

#include "EngineCore/Pipeline/hash_id.h"
#include "SDL3/SDL_gpu.h"

namespace Engine::Extension::OrcaRendererModule::Assets {
    
enum  class RenderPassType
{
    Scene,
    Screen
};

struct RenderPass
{
    RenderPassType Type;

    // input resources is actually a translation map: shader effects would reference a provided name,
    // and get an implementation name out of it that can be used to get the real resource from the 
    // resource database
    struct {
        Core::Pipeline::HashId InterfaceName;
        Core::Pipeline::HashId ImplementationName;
    } InputResources[8];

    // output color targets are much more straight forward, they are a series of textures to be written into
    struct {
        SDL_GPUTexture* Target;
        SDL_GPUTextureFormat Format;
        SDL_FColor ClearColor;
        bool ShouldClear;
    } ColorTargets[8];

    // one depth target ought to be enough for everyone
    struct {
        SDL_GPUTexture* Target;
        float ClearDepth;
        bool ShouldClear;
    } DepthStencilTarget;
};

struct RenderGraphHeader
{
    // this field is hand authored
    size_t Altitude;

    // this count is limited to 0~16
    size_t RenderPassCount;

    inline RenderPass* GetRenderPasses()
    {
        return (RenderPass*)(this + 1);
    }
};

}