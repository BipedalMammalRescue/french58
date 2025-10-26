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

enum class UniformInjectionIdentifier : unsigned char
{
    ModelTransform,
    ViewTransform,
    ProjectionTransform
};

enum class StorageBufferInjectionIdentifier : unsigned char
{
    DirectionalLightBuffer
};

struct InjectedUniform
{
    uint32_t Set;
    uint32_t Binding;
    UniformInjectionIdentifier Identifier;
};

struct InjectedStorageBuffer
{
    uint32_t Set;
    uint32_t Binding;
    StorageBufferInjectionIdentifier Identifier;
};

struct RenderPipeline
{
    SDL_GPUGraphicsPipeline* GraphicsPipeline;

    uint32_t UniformStart;
    uint32_t UniformEnd;

    uint32_t StorageBufferStart;
    uint32_t StorageBufferEnd;
};

} // namespace Engine::Extension::RendererModule::Assets