#include "RendererModule/Assets/vertex_shader.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "RendererModule/renderer_module.h"

#include "EngineCore/Pipeline/asset_enumerable.h"
#include "EngineCore/Runtime/service_table.h"
#include "EngineCore/Runtime/graphics_layer.h"
#include "EngineUtils/ErrorHandling/exceptions.h"

#include <memory>

using namespace Engine::Extension::RendererModule;

Engine::Core::Runtime::CallbackResult Assets::LoadVertexShader(Engine::Core::Pipeline::IAssetEnumerator *inputStreams, Engine::Core::Runtime::ServiceTable *services, void *moduleState)
{
    using namespace Engine::Core::Runtime;

    // reserve memory for the new shaders
    ModuleState* state = static_cast<ModuleState*>(moduleState);
    state->VertexShaders.reserve(state->VertexShaders.size() + inputStreams->Count());

    SDL_GPUShaderStage shaderStage = SDL_GPUShaderStage::SDL_GPU_SHADERSTAGE_VERTEX;

    // load every shader individually
    while (inputStreams->MoveNext()) 
    {
        Engine::Core::Pipeline::RawAsset rawAsset = inputStreams->GetCurrent();

        // load prefixed reflection information
        uint32_t uniformCount = 0;
        uint32_t stroageBufferCount = 0;
        rawAsset.Storage->read((char*)&uniformCount, sizeof(uint32_t));
        rawAsset.Storage->read((char*)&stroageBufferCount, sizeof(uint32_t));

        // load a prefixed length
        size_t codeLength = 0;
        rawAsset.Storage->read((char*)&codeLength, sizeof(size_t));
        
        // load the code
        std::unique_ptr<unsigned char[]> code = std::make_unique<unsigned char[]>(codeLength);
        rawAsset.Storage->read((char*)code.get(), codeLength);
        
        // compile shader
        SDL_GPUShaderCreateInfo shaderInfo = {codeLength,
                                              code.get(),
                                              "main",
                                              SDL_GPU_SHADERFORMAT_SPIRV,
                                              shaderStage,
                                              0,
                                              0,
                                              stroageBufferCount,
                                              uniformCount};

        SDL_GPUShader *newShader = SDL_CreateGPUShader(services->GraphicsLayer->GetDevice(), &shaderInfo);
        
        // TODO: get a default shader for shader failures
        if (newShader == nullptr)
            SE_THROW_GRAPHICS_EXCEPTION;

        state->VertexShaders[rawAsset.ID] = newShader;
    }

    return CallbackSuccess();
}

Engine::Core::Runtime::CallbackResult Assets::UnloadVertexShader(Engine::Core::Pipeline::HashId *ids, size_t count, Core::Runtime::ServiceTable *services, void *moduleState)
{
    ModuleState* state = static_cast<ModuleState*>(moduleState);
    
    for (size_t i = 0; i < count; i++) 
    {
        // TODO: log something for unrecognized data
        auto foundShader = state->VertexShaders.find(ids[i]);
        if (foundShader == state->VertexShaders.end())
            continue;

        // delete shader from GPU
        SDL_ReleaseGPUShader(services->GraphicsLayer->GetDevice(), foundShader->second);

        // erase asset
        state->VertexShaders.erase(foundShader);
    }

    return Core::Runtime::CallbackSuccess();
}