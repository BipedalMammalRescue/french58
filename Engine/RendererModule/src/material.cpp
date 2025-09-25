#include "RendererModule/Assets/material.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineUtils/ErrorHandling/exceptions.h"
#include "RendererModule/renderer_module.h"

#include "EngineCore/Pipeline/asset_enumerable.h"
#include "EngineCore/Runtime/graphics_layer.h"
#include "EngineCore/Runtime/service_table.h"

#include <SDL3/SDL_gpu.h>

using namespace Engine;
using namespace Engine::Extension::RendererModule;

void Assets::LoadMaterial(Core::Pipeline::IAssetEnumerator *inputStreams,
                  Core::Runtime::ServiceTable *services,
                  void *moduleState) 
{
    ModuleState* state = static_cast<ModuleState*>(moduleState);
    state->Materials.reserve(state->Materials.size() + inputStreams->Count());

    while (inputStreams->MoveNext())
    {
        Core::Pipeline::RawAsset asset = inputStreams->GetCurrent();
        std::istream* input = asset.Storage;

        // decode input
        Core::Pipeline::HashId vertShaderId;
        Core::Pipeline::HashId fragShaderId;

        input->read((char*)vertShaderId.Hash.data(), 16);
        input->read((char*)fragShaderId.Hash.data(), 16);

        auto vertShader = state->VertexShaders.find(vertShaderId);
        auto fragShader = state->FragmentShaders.find(fragShaderId);

        if (vertShader == state->VertexShaders.end() || fragShader == state->FragmentShaders.end())
            SE_THROW_GRAPHICS_EXCEPTION;

        SDL_GPUVertexAttribute vertexAttributes[] = {
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

        uint32_t stride = sizeof(float) * 8;

        SDL_GPUVertexBufferDescription vertexBufferDescription {
            0,
            stride,
            SDL_GPUVertexInputRate::SDL_GPU_VERTEXINPUTRATE_VERTEX,
            0
        };

        SDL_GPUVertexInputState vertexInputState {
            &vertexBufferDescription, 
            1, 
            vertexAttributes, 
            3
        };

        SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo {
            vertShader->second,
            fragShader->second,
            vertexInputState,
            SDL_GPUPrimitiveType::SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        };

        pipelineCreateInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;

        SDL_GPUColorTargetDescription colorTarget {
            SDL_GetGPUSwapchainTextureFormat(
                services->GraphicsLayer->GetDevice(), 
                services->GraphicsLayer->GetWindow()
            )
        };

        pipelineCreateInfo.target_info = {&colorTarget, 1};

        SDL_GPUGraphicsPipeline *newPipeline = SDL_CreateGPUGraphicsPipeline(
            services->GraphicsLayer->GetDevice(), 
            &pipelineCreateInfo);
        
        if (newPipeline == nullptr)
            SE_THROW_GRAPHICS_EXCEPTION;

        state->Materials[asset.ID] = newPipeline;
    }
}
            
void Assets::UnloadMaterial(Core::Pipeline::HashId *ids, size_t count,
                          Core::Runtime::ServiceTable *services, void *moduleState)
{
    ModuleState* state = static_cast<ModuleState*>(moduleState);
    
    for (size_t i = 0; i < count; i++)
    {
        auto foundPipeline = state->Materials.find(ids[i]);
        if (foundPipeline == state->Materials.end())
            continue;

        SDL_ReleaseGPUGraphicsPipeline(services->GraphicsLayer->GetDevice(), foundPipeline->second);
        state->Materials.erase(foundPipeline);
    }
}