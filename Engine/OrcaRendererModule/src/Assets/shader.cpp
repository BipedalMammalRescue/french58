#include "OrcaRendererModule/Assets/shader.h"
#include "EngineCore/AssetManagement/asset_loading_context.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/graphics_layer.h"
#include "OrcaRendererModule/orca_renderer_module.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_gpu.h"

using namespace Engine::Extension::OrcaRendererModule::Assets;

Engine::Core::Runtime::CallbackResult Engine::Extension::OrcaRendererModule::Assets::ContextualizeShader(
    Engine::Core::Runtime::ServiceTable *services, void *moduleState, Engine::Core::AssetManagement::AssetLoadingContext* outContext, size_t contextCount)
{
    Engine::Extension::OrcaRendererModule::ModuleState* state = static_cast<Engine::Extension::OrcaRendererModule::ModuleState*>(moduleState);

    for (size_t i = 0; i < contextCount; i++)
    {
        // check if the shader already exists
        if (!outContext[i].ReplaceExisting && state->GetShader(outContext[i].AssetId) != nullptr)
        {
            state->GetLogger()->Information("Shader {} is already loaded.", outContext[i].AssetId);
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

Engine::Core::Runtime::CallbackResult Engine::Extension::OrcaRendererModule::Assets::IndexShader(
    Engine::Core::Runtime::ServiceTable *services, void *moduleState, Engine::Core::AssetManagement::AssetLoadingContext* inContext)
{
    Engine::Extension::OrcaRendererModule::ModuleState* state = static_cast<Engine::Extension::OrcaRendererModule::ModuleState*>(moduleState);

    // get metadata from the asset
    unsigned char* assetFile = (unsigned char*)services->TransientAllocator->GetBuffer(inContext->Buffer.Location.TransientBufferId);
    ShaderStage stage = *(ShaderStage*)assetFile;
    uint32_t numSamplers = *(uint32_t*)assetFile + 1;
    uint32_t numStorageTextures = *(uint32_t*)assetFile + 5;
    uint32_t numStorageBuffers = *(uint32_t*)assetFile + 9;
    uint32_t numUniformBuffers = *(uint32_t*)assetFile + 13;
    unsigned char* byteCode = assetFile + 17;

    // match the shader stage
    SDL_GPUShaderStage sdlStage;
    switch (stage)
    {
    case ShaderStage::Vertex:
        sdlStage = SDL_GPU_SHADERSTAGE_VERTEX;
        break;
    case ShaderStage::Fragment:
        sdlStage = SDL_GPU_SHADERSTAGE_FRAGMENT;
        break;
    default:
        state->GetLogger()->Error("Shader {} has invalid stage.", inContext->AssetId);
        return Engine::Core::Runtime::CallbackSuccess();
    }

    // compile the shader (still hard code a bunch of things because I don't have time to care)
    // NOTE: this needs to be eventually moved into the graphics layer
    SDL_GPUShaderCreateInfo createInfo {
        inContext->SourceSize - 1,
        byteCode,
        "main",
        SDL_GPU_SHADERFORMAT_SPIRV,
        sdlStage,
        numSamplers,
        numStorageTextures,
        numStorageBuffers,
        numUniformBuffers,
    };

    SDL_GPUShader* shader = SDL_CreateGPUShader(services->GraphicsLayer->GetDevice(), &createInfo);
    if (shader == nullptr)
    {
        state->GetLogger()->Error("Failed to create shader {}, detail: {}", inContext->AssetId, SDL_GetError());
        return Engine::Core::Runtime::CallbackSuccess();
    }

    // register the shader in the module state
    state->AddOrReplaceShader(inContext->AssetId, shader);
    return Engine::Core::Runtime::CallbackSuccess();
}