#pragma once

#include "EngineCore/Pipeline/fwd.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Runtime/fwd.h"
#include "SDL3/SDL_gpu.h"

namespace Engine::Extension::RendererModule::Assets {

struct Material 
{
    SDL_GPUGraphicsPipeline* Pipeline;
    SDL_GPURenderPass* RenderPass;
};

void LoadMaterial(Core::Pipeline::AssetEnumerable *inputStreams,
                        Core::Runtime::ServiceTable *services,
                        void *moduleState);
                        
void UnloadMaterial(Core::Pipeline::HashId *ids, size_t count,
                          Core::Runtime::ServiceTable *services, void *moduleState);

}