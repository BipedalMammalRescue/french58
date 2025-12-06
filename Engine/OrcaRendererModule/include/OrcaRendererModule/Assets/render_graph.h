#pragma once

#include "EngineCore/Pipeline/hash_id.h"
#include "SDL3/SDL_gpu.h"

namespace Engine::Extension::OrcaRendererModule::Assets {

enum class RenderPassType
{
    Scene,
    Screen
};

// Collects information about how to begin a render pass, including color targets and depth-stencil targets.
// This structure is normalized in size so they can be crammed inside a variable sized array inside the asset.
// Hopefully the disk space usage can be subsidized by the uniformity of the content.
// TODO: need to redesign the way it references the texture assets; in this case it should straight up not use hash id
// for the color targets and instead use something smaller than 64 bytes, then direclty indexed into the corresponding
// data structure used by the graph, as these names are local to the graph-pass relationship.
struct RenderPass
{
    // cram the metadata into one dword
    RenderPassType Type;
    unsigned char InputResourceCount;
    unsigned char ColorTargetCount;

    // input resources is actually a translation map: shader effects would reference a provided name,
    // and get an implementation name out of it that can be used to get the real resource from the
    // resource database
    struct
    {
        Core::Pipeline::HashId InterfaceName;
        Core::Pipeline::HashId ImplementationName;
    } InputResources[8];

    // output color targets are much more straight forward, they are a series of textures to be written into
    struct
    {
        SDL_GPUTexture *Target;
        SDL_GPUTextureFormat Format;
        SDL_FColor ClearColor;
        bool ShouldClear;
    } ColorTargets[8];

    // one depth target ought to be enough for everyone
    struct
    {
        SDL_GPUTexture *Target;
        float ClearDepth;
        bool ShouldClear;
    } DepthStencilTarget;
};

struct RenderGraph
{
    // this field is hand authored
    size_t Altitude;

    // this count is limited to 0~16
    size_t RenderPassCount;

    inline RenderPass *GetRenderPasses()
    {
        return (RenderPass *)(this + 1);
    }
};

} // namespace Engine::Extension::OrcaRendererModule::Assets