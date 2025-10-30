#pragma once

#include <EngineCore/Pipeline/fwd.h>
#include <EngineCore/Pipeline/hash_id.h>
#include <EngineCore/Runtime/crash_dump.h>
#include <EngineCore/Runtime/fwd.h>
#include <SDL3/SDL_gpu.h>
#include <cstddef>

namespace Engine::Extension::RendererModule::Assets {

Core::Runtime::CallbackResult LoadRenderPipeline(Core::Pipeline::IAssetEnumerator *inputStreams,
                        Core::Runtime::ServiceTable *services,
                        void *moduleState);
                        
Core::Runtime::CallbackResult UnloadRenderPipeline(Core::Pipeline::HashId *ids, size_t count,
                          Core::Runtime::ServiceTable *services, void *moduleState);

void DisposeRenderPipeline(Core::Runtime::ServiceTable *services, SDL_GPUShader* shader);

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

struct InjectedDataSet
{
    uint32_t UniformStart;
    uint32_t UniformEnd;
    uint32_t StorageBufferStart;
    uint32_t StorageBufferEnd;
};

struct RenderPipeline
{
    SDL_GPUGraphicsPipeline* GraphicsPipeline;
    Core::Pipeline::HashId PrototypeId;

    InjectedDataSet StaticVertex;
    InjectedDataSet StaticFragment;
    InjectedDataSet DynamicVertex;
    InjectedDataSet DynamicFragment;
};

} // namespace Engine::Extension::RendererModule::Assets