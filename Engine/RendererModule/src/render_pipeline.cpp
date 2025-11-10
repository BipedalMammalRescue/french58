#include "RendererModule/Assets/render_pipeline.h"
#include "RendererModule/renderer_module.h"

#include <EngineCore/Logging/logger_service.h>
#include <EngineCore/Pipeline/hash_id.h>
#include <EngineCore/Runtime/crash_dump.h>
#include <EngineUtils/ErrorHandling/exceptions.h>
#include <EngineCore/Pipeline/asset_enumerable.h>
#include <EngineCore/Runtime/graphics_layer.h>
#include <EngineCore/Runtime/service_table.h>

#include <SDL3/SDL_gpu.h>

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

Engine::Core::Runtime::CallbackResult Assets::LoadRenderPipeline(Core::Pipeline::IAssetEnumerator *inputStreams,
                                                                 Core::Runtime::ServiceTable *services,
                                                                 void *moduleState)
{
    const char* channels[] = {"RenderPipelineLoader"};
    Core::Logging::Logger logger = services->LoggerService->CreateLogger(channels, 1);

    ModuleState* state = static_cast<ModuleState*>(moduleState);
    while (inputStreams->MoveNext())
    {
        RenderPipeline pipeline;

        // find the shaders and compile them into a pipeline
        Core::Pipeline::HashId vertexShaderId;
        Core::Pipeline::HashId fragmentShaderId;
        inputStreams->GetCurrent().Storage->read((char*)&vertexShaderId, sizeof(vertexShaderId));
        inputStreams->GetCurrent().Storage->read((char*)&fragmentShaderId, sizeof(fragmentShaderId));

        // notify the prototype id
        inputStreams->GetCurrent().Storage->read((char*)&pipeline.PrototypeId, sizeof(pipeline.PrototypeId));

        auto foundVertShader = state->VertexShaders.find(vertexShaderId);
        auto foundFragShader = state->FragmentShaders.find(fragmentShaderId);
        if (foundVertShader == state->VertexShaders.end() || foundFragShader == state->FragmentShaders.end())
        {
            logger.Error("Error loading pipeline {pipelineId}, shaders (fragment = {frag}, vertex = {vert}) not found.", 
                {inputStreams->GetCurrent().ID, fragmentShaderId, vertexShaderId});
            continue;
        }

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

        pipeline.GraphicsPipeline = SDL_CreateGPUGraphicsPipeline(services->GraphicsLayer->GetDevice(), &pipelineCreateInfo);

        // static vertex uniform
        pipeline.StaticVertex.UniformStart = state->InjectedUniforms.size();
        size_t staticVertexUniformCount = 0;
        inputStreams->GetCurrent().Storage->read((char*)&staticVertexUniformCount, sizeof(size_t));
        for (size_t i = 0; i < staticVertexUniformCount; i++)
        {
            Assets::InjectedUniform newInjectedUniform;
            inputStreams->GetCurrent().Storage->read((char*)&newInjectedUniform.Binding, sizeof(newInjectedUniform.Binding));
            inputStreams->GetCurrent().Storage->read((char*)&newInjectedUniform.Identifier, sizeof(newInjectedUniform.Identifier));
            state->InjectedUniforms.push_back(newInjectedUniform);
        }
        pipeline.StaticVertex.UniformEnd = state->InjectedUniforms.size();

        // static fragment uniform
        pipeline.StaticFragment.UniformStart = state->InjectedUniforms.size();
        size_t staticFragmentUniformCount = 0;
        inputStreams->GetCurrent().Storage->read((char*)&staticFragmentUniformCount, sizeof(size_t));
        for (size_t i = 0; i < staticFragmentUniformCount; i++)
        {
            Assets::InjectedUniform newInjectedUniform;
            inputStreams->GetCurrent().Storage->read((char*)&newInjectedUniform.Binding, sizeof(newInjectedUniform.Binding));
            inputStreams->GetCurrent().Storage->read((char*)&newInjectedUniform.Identifier, sizeof(newInjectedUniform.Identifier));
            state->InjectedUniforms.push_back(newInjectedUniform);
        }
        pipeline.StaticFragment.UniformEnd = state->InjectedUniforms.size();

        // dynamic vertex uniform
        pipeline.DynamicVertex.UniformStart = state->InjectedUniforms.size();
        size_t DynamicVertexUniformCount = 0;
        inputStreams->GetCurrent().Storage->read((char*)&DynamicVertexUniformCount, sizeof(size_t));
        for (size_t i = 0; i < DynamicVertexUniformCount; i++)
        {
            Assets::InjectedUniform newInjectedUniform;
            inputStreams->GetCurrent().Storage->read((char*)&newInjectedUniform.Binding, sizeof(newInjectedUniform.Binding));
            inputStreams->GetCurrent().Storage->read((char*)&newInjectedUniform.Identifier, sizeof(newInjectedUniform.Identifier));
            state->InjectedUniforms.push_back(newInjectedUniform);
        }
        pipeline.DynamicVertex.UniformEnd = state->InjectedUniforms.size();

        // dynamic fragment uniform
        pipeline.DynamicFragment.UniformStart = state->InjectedUniforms.size();
        size_t DynamicFragmentUniformCount = 0;
        inputStreams->GetCurrent().Storage->read((char*)&DynamicFragmentUniformCount, sizeof(size_t));
        for (size_t i = 0; i < DynamicFragmentUniformCount; i++)
        {
            Assets::InjectedUniform newInjectedUniform;
            inputStreams->GetCurrent().Storage->read((char*)&newInjectedUniform.Binding, sizeof(newInjectedUniform.Binding));
            inputStreams->GetCurrent().Storage->read((char*)&newInjectedUniform.Identifier, sizeof(newInjectedUniform.Identifier));
            state->InjectedUniforms.push_back(newInjectedUniform);
        }
        pipeline.DynamicFragment.UniformEnd = state->InjectedUniforms.size();

        // static vertex storage buffer
        pipeline.StaticVertex.StorageBufferStart = state->InjectedStorageBuffers.size();
        size_t staticVertexBufferCount = 0;
        inputStreams->GetCurrent().Storage->read((char*)&staticVertexBufferCount, sizeof(size_t));
        for (size_t i = 0; i < staticVertexBufferCount; i++)
        {
            Assets::InjectedStorageBuffer newInjectedBuffer;
            inputStreams->GetCurrent().Storage->read((char*)&newInjectedBuffer.Binding, sizeof(newInjectedBuffer.Binding));
            inputStreams->GetCurrent().Storage->read((char*)&newInjectedBuffer.Identifier, sizeof(newInjectedBuffer.Identifier));
            state->InjectedStorageBuffers.push_back(newInjectedBuffer);
        }
        pipeline.StaticVertex.StorageBufferEnd = state->InjectedStorageBuffers.size();

        // static fragment storage buffer
        pipeline.StaticFragment.StorageBufferStart = state->InjectedStorageBuffers.size();
        size_t staticFragmentBufferCount = 0;
        inputStreams->GetCurrent().Storage->read((char*)&staticFragmentBufferCount, sizeof(size_t));
        for (size_t i = 0; i < staticFragmentBufferCount; i++)
        {
            Assets::InjectedStorageBuffer newInjectedBuffer;
            inputStreams->GetCurrent().Storage->read((char*)&newInjectedBuffer.Binding, sizeof(newInjectedBuffer.Binding));
            inputStreams->GetCurrent().Storage->read((char*)&newInjectedBuffer.Identifier, sizeof(newInjectedBuffer.Identifier));
            state->InjectedStorageBuffers.push_back(newInjectedBuffer);
        }
        pipeline.StaticFragment.StorageBufferEnd = state->InjectedStorageBuffers.size();

        // dynamic vertex storage buffer
        pipeline.DynamicVertex.StorageBufferStart = state->InjectedStorageBuffers.size();
        size_t DynamicVertexBufferCount = 0;
        inputStreams->GetCurrent().Storage->read((char*)&DynamicVertexBufferCount, sizeof(size_t));
        for (size_t i = 0; i < DynamicVertexBufferCount; i++)
        {
            Assets::InjectedStorageBuffer newInjectedBuffer;
            inputStreams->GetCurrent().Storage->read((char*)&newInjectedBuffer.Binding, sizeof(newInjectedBuffer.Binding));
            inputStreams->GetCurrent().Storage->read((char*)&newInjectedBuffer.Identifier, sizeof(newInjectedBuffer.Identifier));
            state->InjectedStorageBuffers.push_back(newInjectedBuffer);
        }
        pipeline.DynamicVertex.StorageBufferEnd = state->InjectedStorageBuffers.size();

        // dynamic fragment storage buffer
        pipeline.DynamicFragment.StorageBufferStart = state->InjectedStorageBuffers.size();
        size_t DynamicFragmentBufferCount = 0;
        inputStreams->GetCurrent().Storage->read((char*)&DynamicFragmentBufferCount, sizeof(size_t));
        for (size_t i = 0; i < DynamicFragmentBufferCount; i++)
        {
            Assets::InjectedStorageBuffer newInjectedBuffer;
            inputStreams->GetCurrent().Storage->read((char*)&newInjectedBuffer.Binding, sizeof(newInjectedBuffer.Binding));
            inputStreams->GetCurrent().Storage->read((char*)&newInjectedBuffer.Identifier, sizeof(newInjectedBuffer.Identifier));
            state->InjectedStorageBuffers.push_back(newInjectedBuffer);
        }
        pipeline.DynamicFragment.StorageBufferEnd = state->InjectedStorageBuffers.size();

        state->PipelineIndex[inputStreams->GetCurrent().ID] = state->Pipelines.size();
        state->Pipelines.push_back({pipeline});
    }

    return Core::Runtime::CallbackSuccess();
}

Engine::Core::Runtime::CallbackResult Assets::UnloadRenderPipeline(Core::Pipeline::HashId *ids, size_t count, Core::Runtime::ServiceTable *services, void *moduleState)
{
    // TODO: implement unloading (asset unloading isn't yet performed)
    return Core::Runtime::Crash(__FILE__, __LINE__, "Unloading rendering pipeline not yet implemented!");
}