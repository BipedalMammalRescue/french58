#pragma once

#include "EngineCore/Pipeline/fwd.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/fwd.h"
#include "SDL3/SDL_gpu.h"

namespace Engine::Extension::RendererModule::Assets {

struct Material 
{
    SDL_GPUGraphicsPipeline* Pipeline;
    SDL_GPURenderPass* RenderPass;
};

Core::Runtime::CallbackResult LoadMaterial(Core::Pipeline::IAssetEnumerator *inputStreams,
                        Core::Runtime::ServiceTable *services,
                        void *moduleState);
                        
Core::Runtime::CallbackResult UnloadMaterial(Core::Pipeline::HashId *ids, size_t count,
                          Core::Runtime::ServiceTable *services, void *moduleState);

}