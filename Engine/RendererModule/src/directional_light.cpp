#include "RendererModule/Components/directional_light.h"
#include "EngineUtils/Memory/memstream_lite.h"
#include "RendererModule/renderer_module.h"

#include <EngineCore/Pipeline/component_definition.h>
#include <EngineCore/Pipeline/hash_id.h>
#include <EngineCore/Pipeline/variant.h>
#include <EngineCore/Runtime/crash_dump.h>
#include <EngineCore/Runtime/service_table.h>
#include <EngineCore/Runtime/graphics_layer.h>

#include <SDL3/SDL_gpu.h>
#include <md5.h>

using namespace Engine::Extension::RendererModule;

bool Components::CompileDirectionalLight(Core::Pipeline::RawComponent input, std::ostream* output)
{
    if (input.FieldC != 2)
        return false;

    DirectionalLight result { glm::vec3(1, 1, 1), 0, glm::vec3(0, 0, 0), 0 };

    for (int i = 0; i < 3; i++)
    {
        if (i > 2)
            return false;

        Core::Pipeline::Field field = input.FieldV[i];
        if (field.Name == Core::Pipeline::HashId(md5::compute("Direction")))
        {
            if (field.Payload.Type != Core::Pipeline::VariantType::Vec3)
                return false;
            result.Direction = field.Payload.Data.Vec3;
            break;
        }
    }

    for (int i = 0; i < 3; i++)
    {
        if (i > 2)
            return false;

        Core::Pipeline::Field field = input.FieldV[i];
        if (field.Name == Core::Pipeline::HashId(md5::compute("Color")))
        {
            if (field.Payload.Type != Core::Pipeline::VariantType::Vec3)
                return false;
            result.Color = field.Payload.Data.Vec3;
            break;
        }
    }

    output->write((char*)&result, sizeof(result));
    return true;
}

Engine::Core::Runtime::CallbackResult Components::LoadDirectionalLight(size_t count, Utils::Memory::MemStreamLite& stream, Core::Runtime::ServiceTable* services, void* moduleState)
{
    RendererModuleState* state = static_cast<RendererModuleState*>(moduleState);
    state->DirectionalLights.reserve(state->DirectionalLights.size() + count);
    
    // load the reference copy
    DirectionalLight buffer;
    for (size_t i = 0; i < count; i++)
    {
        state->DirectionalLights.push_back(stream.Read<DirectionalLight>());
    }

    // TODO: 1. move the buffer upload logic into graphics layer; 2. add error handling, crash the app whenever SDL is reporting error

    // adjust GPU buffer
    // TODO: currenlty it's a tight fit, eventually we'll need some way to pre-allocate extra buffer size for adding lights in game
    if (state->DirectionalLightBuffer != nullptr)
    {
        SDL_ReleaseGPUBuffer(services->GraphicsLayer->GetDevice(), state->DirectionalLightBuffer);
        state->DirectionalLightBuffer = nullptr;
    }

    uint32_t directionalLightBufferSize = (uint32_t)(sizeof(DirectionalLight) * state->DirectionalLights.size());

    SDL_GPUBufferCreateInfo createInfo {
        SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
        directionalLightBufferSize
    };

    SDL_GPUBuffer* directionalLightBuffer = SDL_CreateGPUBuffer(services->GraphicsLayer->GetDevice(), &createInfo);

    auto command = SDL_AcquireGPUCommandBuffer(services->GraphicsLayer->GetDevice());
    auto pass = SDL_BeginGPUCopyPass(command);

    SDL_GPUTransferBufferCreateInfo transBufferCreateInfo 
    {
        SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        directionalLightBufferSize
    };

    SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(
        services->GraphicsLayer->GetDevice(),
        &transBufferCreateInfo
    );

    // upload the data to transfer buffer
    void* mapping = SDL_MapGPUTransferBuffer(
        services->GraphicsLayer->GetDevice(),
        transferBuffer,
        false
    );
    memcpy(mapping, state->DirectionalLights.data(), directionalLightBufferSize);
    SDL_UnmapGPUTransferBuffer(services->GraphicsLayer->GetDevice(), transferBuffer);

    SDL_GPUTransferBufferLocation transBufLocation 
    {
        transferBuffer,
        0
    };

    // copy from transfer buffer to the destination location
    SDL_GPUBufferRegion directionalLightBufferRegion
    {
        directionalLightBuffer,
        0,
        directionalLightBufferSize
    };

    SDL_UploadToGPUBuffer(
        pass,
        &transBufLocation,
        &directionalLightBufferRegion,
        false
    );

    SDL_EndGPUCopyPass(pass);
    SDL_SubmitGPUCommandBuffer(command);
    SDL_ReleaseGPUTransferBuffer(services->GraphicsLayer->GetDevice(), transferBuffer);

    state->DirectionalLightBuffer = directionalLightBuffer;
    return Core::Runtime::CallbackSuccess();
}