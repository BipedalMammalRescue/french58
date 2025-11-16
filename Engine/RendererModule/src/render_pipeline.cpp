#include "RendererModule/Assets/render_pipeline.h"
#include "EngineCore/AssetManagement/asset_loading_context.h"
#include "EngineCore/Runtime/heap_allocator.h"
#include "EngineUtils/Memory/memstream_lite.h"
#include "RendererModule/renderer_module.h"
#include "RendererModule/common.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/graphics_layer.h"
#include "EngineCore/Runtime/service_table.h"

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_error.h"

using namespace Engine::Extension::RendererModule;

// TODO: at some point we need to make this configurable
const SDL_GPUVertexAttribute VertexAttributes[] = {
    {
        0,
        0,
        SDL_GPUVertexElementFormat::SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
        0
    },
    {
        1,
        0,
        SDL_GPUVertexElementFormat::SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
        sizeof(float) * 3
    },
    {
        2,
        0,
        SDL_GPUVertexElementFormat::SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
        sizeof(float) * 6
    }
};

const SDL_GPUVertexBufferDescription vertexBufferDescription {
    0,
    sizeof(float) * 8,
    SDL_GPUVertexInputRate::SDL_GPU_VERTEXINPUTRATE_VERTEX,
    0
};

const SDL_GPUVertexInputState vertexInputState {
    &vertexBufferDescription, 
    1, 
    VertexAttributes, 
    3
};

Engine::Core::Runtime::CallbackResult Assets::ContextualizeRenderPipeline(Engine::Core::Runtime::ServiceTable *services, void *moduleState, Engine::Core::AssetManagement::AssetLoadingContext* outContext, size_t contextCount)
{
    // calculate the total size needed
    RendererModuleState* state = static_cast<RendererModuleState*>(moduleState);
    for (size_t i = 0; i < contextCount; i++)
    {
        if (!state->PipelineIndex.TryInsert({outContext[i].AssetId}) && !outContext->ReplaceExisting)
        {
            state->Logger.Information("Render pipeline {} is already loaded.", outContext[i].AssetId);
            outContext[i].Buffer.Type = Core::AssetManagement::LoadBufferType::Invalid;
        }
        else
        {
            outContext[i].Buffer.Type = Engine::Core::AssetManagement::LoadBufferType::ModuleBuffer;
        }
    }

    // distribute out the pointers
    for (size_t i = 0; i < contextCount; i++)
    {
        if (outContext[i].Buffer.Type == Core::AssetManagement::LoadBufferType::Invalid)
            continue;
        outContext[i].Buffer.Location.ModuleBuffer = services->HeapAllocator->Allocate(outContext->SourceSize);
    }

    return Engine::Core::Runtime::CallbackSuccess();
}

template <typename T>
Engine::Extension::RendererModule::Assets::InjectedDataAddress LocateInjectedDataFromStream(Engine::Utils::Memory::MemStreamLite& stream)
{
    size_t count = stream.Read<size_t>();
    size_t offset = stream.GetPosition();
    stream.Seek(offset + count * sizeof(T));

    return { count, offset};
}

Engine::Core::Runtime::CallbackResult Assets::IndexRenderPipeline(Engine::Core::Runtime::ServiceTable *services, void *moduleState, Engine::Core::AssetManagement::AssetLoadingContext* inContext)
{
    RendererModuleState* state = static_cast<RendererModuleState*>(moduleState);
    Assets::RenderPipelineHeader* header = static_cast<Assets::RenderPipelineHeader*>(inContext->Buffer.Location.ModuleBuffer);

    // find shaders
    auto foundVertShader = state->VertexShaders.find(header->VertexShader);
    auto foundFragShader = state->FragmentShaders.find(header->FragmentShader);
    if (foundVertShader == state->VertexShaders.end() || foundFragShader == state->FragmentShaders.end())
    {
        state->Logger.Error("Failed to find shaders for render pipeline {}.", inContext->AssetId);

        // this asset can't really be deleted at this moment
        Assets::RenderPipeline pipeline = { inContext->AssetId, nullptr, header };
        state->PipelineIndex.InsertRange(&pipeline, 1);
        return Core::Runtime::CallbackSuccess();
    }

    // create gpu pipeline
    SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo {
        foundVertShader->second,
        foundFragShader->second,
        vertexInputState,
        SDL_GPUPrimitiveType::SDL_GPU_PRIMITIVETYPE_TRIANGLELIST
    };

    pipelineCreateInfo.depth_stencil_state.enable_depth_test = true;
    pipelineCreateInfo.depth_stencil_state.enable_depth_write = true;
    pipelineCreateInfo.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;

    pipelineCreateInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;

    SDL_GPUColorTargetDescription colorTarget {
        SDL_GetGPUSwapchainTextureFormat(
            services->GraphicsLayer->GetDevice(), 
            services->GraphicsLayer->GetWindow()
        )
    };
    pipelineCreateInfo.target_info = {
        &colorTarget, 
        1, 
        SDL_GPU_TEXTUREFORMAT_D32_FLOAT, 
        true
    };

    auto gpuPipeline = SDL_CreateGPUGraphicsPipeline(services->GraphicsLayer->GetDevice(), &pipelineCreateInfo);
    if (gpuPipeline == nullptr)
    {
        state->Logger.Error("Failed to create gpu graphics pipeline for render pipeline {}, detail:", inContext->AssetId, SDL_GetError());
    }

    // read the file
    Utils::Memory::MemStreamLite stream { SkipHeader(header), 0 };
    Assets::RenderPipeline pipeline = { 
        inContext->AssetId, 
        gpuPipeline, 
        header,
        LocateInjectedDataFromStream<InjectedUniform>(stream),
        LocateInjectedDataFromStream<InjectedUniform>(stream),
        LocateInjectedDataFromStream<InjectedUniform>(stream),
        LocateInjectedDataFromStream<InjectedUniform>(stream),
        LocateInjectedDataFromStream<InjectedStorageBuffer>(stream),
        LocateInjectedDataFromStream<InjectedStorageBuffer>(stream),
        LocateInjectedDataFromStream<InjectedStorageBuffer>(stream),
        LocateInjectedDataFromStream<InjectedStorageBuffer>(stream),
    };

    // NOTE: non-replace behavior would have been intercepted beforehand
    state->PipelineIndex.Replace(pipeline);
    return Engine::Core::Runtime::CallbackSuccess();
}
