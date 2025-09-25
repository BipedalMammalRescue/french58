#pragma once

#include "EngineCore/Pipeline/fwd.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Runtime/fwd.h"
#include "SDL3/SDL_gpu.h"

#include <cstddef>

namespace Engine::Extension::RendererModule::Assets {

void LoadFragmentShader(Core::Pipeline::IAssetEnumerator *inputStreams,
                        Core::Runtime::ServiceTable *services,
                        void *moduleState);
                        
void UnloadFragmentShader(Core::Pipeline::HashId *ids, size_t count,
                          Core::Runtime::ServiceTable *services, void *moduleState);

void DisposeFragmentShader(Core::Runtime::ServiceTable *services, SDL_GPUShader* shader);

} // namespace Engine::Extension::RendererModule::Assets