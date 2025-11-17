#pragma once


#include "EngineCore/Pipeline/hash_id.h"
#include <EngineCore/Pipeline/fwd.h>
#include <EngineCore/Pipeline/asset_definition.h>
#include <EngineCore/Runtime/crash_dump.h>
#include <EngineCore/Runtime/fwd.h>
#include <SDL3/SDL_gpu.h>
#include <cstddef>

namespace Engine::Extension::RendererModule::Assets {

Core::Runtime::CallbackResult ContextualizeRenderPipeline(Core::Runtime::ServiceTable *services, void *moduleState, Core::AssetManagement::AssetLoadingContext* outContext, size_t contextCount);
Core::Runtime::CallbackResult IndexRenderPipeline(Core::Runtime::ServiceTable *services, void *moduleState, Core::AssetManagement::AssetLoadingContext* inContext);

enum class DynamicUniformIdentifier : unsigned char
{
    ModelTransform,
    ViewTransform,
    ProjectionTransform
};

enum class StaticStorageBufferIdentifier : unsigned char
{
    DirectionalLightBuffer
};

struct InjectedUniform
{
    uint32_t Binding;
    unsigned char Identifier;
};

struct InjectedStorageBuffer
{
    uint32_t Binding;
    unsigned char Identifier;
};

struct InjectedDataAddress
{
    size_t Count;
    size_t Offset;
};

struct RenderPipelineHeader
{
    Core::Pipeline::HashId VertexShader;
    Core::Pipeline::HashId FragmentShader;
    Core::Pipeline::HashId PrototypeId;
};

struct RenderPipeline
{
    Core::Pipeline::HashId Id;
    SDL_GPUGraphicsPipeline* GpuPipeline;
    RenderPipelineHeader* Header;

    InjectedDataAddress StaticVertUniform;
    InjectedDataAddress StaticFragUniform;

    InjectedDataAddress DynamicVertUniform;
    InjectedDataAddress DynamicFragUniform;

    InjectedDataAddress StaticVertStorageBuffer;
    InjectedDataAddress StaticFragStorageBuffer;

    InjectedDataAddress DynamicVertStorageBuffer;
    InjectedDataAddress DynamicFragStorageBuffer;
};

// renderer pipelines are only sorted based on their asset id
struct RenderPipelineComparer
{
    static int Compare(const RenderPipeline* a, const RenderPipeline* b)
    {
        if (a->Id < b->Id)
            return -1;
        if (a->Id > b->Id)
            return 1;
        return 0;
    }
};

} // namespace Engine::Extension::RendererModule::Assets