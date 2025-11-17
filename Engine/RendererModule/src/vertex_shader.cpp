#include "RendererModule/Assets/vertex_shader.h"
#include "EngineCore/AssetManagement/asset_loading_context.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineUtils/Memory/memstream_lite.h"
#include "RendererModule/renderer_module.h"
#include "EngineCore/Runtime/service_table.h"
#include "EngineCore/Runtime/graphics_layer.h"
#include "SDL3/SDL_error.h"

#include <cstdint>

using namespace Engine::Extension::RendererModule;

Engine::Core::Runtime::CallbackResult Assets::ContextualizeVertexShader(
    Engine::Core::Runtime::ServiceTable *services, void *moduleState, Engine::Core::AssetManagement::AssetLoadingContext* outContext, size_t contextCount)
{
    // reserve memory for the new shaders
    RendererModuleState* state = static_cast<RendererModuleState*>(moduleState);
    state->VertexShaders.reserve(state->VertexShaders.size() + contextCount);
    
    SDL_GPUShaderStage shaderStage = SDL_GPUShaderStage::SDL_GPU_SHADERSTAGE_VERTEX;

    // we need to use transient buffer for this
    for (size_t i = 0; i < contextCount; i++)
    {
        if (state->VertexShaders.find(outContext[i].AssetId) != state->VertexShaders.end())
        {
            state->Logger.Information("Vertex shader {} already loaded (reloading shader is not yet supported).", outContext[i].AssetId);
            outContext[i].Buffer.Type = Engine::Core::AssetManagement::LoadBufferType::Invalid;
        }
        else
        {
            outContext[i].Buffer.Type = Engine::Core::AssetManagement::LoadBufferType::TransientBuffer;
            outContext[i].Buffer.Location.TransientBufferSize = outContext[i].SourceSize;
        }
    }

    return Engine::Core::Runtime::CallbackSuccess();
}

Engine::Core::Runtime::CallbackResult Assets::IndexVertexShader(
    Engine::Core::Runtime::ServiceTable *services, void *moduleState, Engine::Core::AssetManagement::AssetLoadingContext* inContext)
{
    using namespace Engine::Core::Runtime;
    RendererModuleState* state = static_cast<RendererModuleState*>(moduleState);

    void* buffer = services->TransientAllocator->GetBuffer(inContext->Buffer.Location.TransientBufferId);
    if (buffer == nullptr)
    {
        state->Logger.Error("Failed to load vertex shader {} because transient buffer is invalid.");
        return CallbackSuccess();
    }

    Engine::Utils::Memory::MemStreamLite stream { buffer, 0 };

    // load prefixed reflection information
    uint32_t uniformCount = stream.Read<uint32_t>();
    uint32_t stroageBufferCount = stream.Read<uint32_t>();

    // load a prefixed length
    size_t codeLength = stream.Read<size_t>();

    // compile shader
    SDL_GPUShaderCreateInfo shaderInfo = {codeLength,
                                            (unsigned char*)stream.Buffer + stream.Cursor,
                                            "main",
                                            SDL_GPU_SHADERFORMAT_SPIRV,
                                            SDL_GPUShaderStage::SDL_GPU_SHADERSTAGE_VERTEX,
                                            0,
                                            0,
                                            stroageBufferCount,
                                            uniformCount};

    SDL_GPUShader *newShader = SDL_CreateGPUShader(services->GraphicsLayer->GetDevice(), &shaderInfo);

    if (newShader == nullptr)
    {
        state->Logger.Information("Failed to compile vertex shader {} on the GPU, detail: {}", inContext->AssetId, SDL_GetError());
        return CallbackSuccess();
    }

    state->VertexShaders[inContext->AssetId] = newShader;
    return CallbackSuccess();
}