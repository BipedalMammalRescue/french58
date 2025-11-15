#pragma once

#include "EngineCore/AssetManagement/asset_loading_context.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/fwd.h"
#include "SDL3/SDL_gpu.h"

#include <cstddef>

namespace Engine::Extension::RendererModule::Assets {

Engine::Core::Runtime::CallbackResult ContextualizeFragmentShader(
    Engine::Core::Runtime::ServiceTable *services, void *moduleState, Engine::Core::AssetManagement::AssetLoadingContext* outContext, size_t contextCount);

Engine::Core::Runtime::CallbackResult IndexFragmentShader(
    Engine::Core::Runtime::ServiceTable *services, void *moduleState, Engine::Core::AssetManagement::AssetLoadingContext* inContext);

void DisposeFragmentShader(Core::Runtime::ServiceTable *services, SDL_GPUShader *shader);

} // namespace Engine::Extension::RendererModule::Assets