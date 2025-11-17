#pragma once

#include "EngineCore/AssetManagement/asset_loading_context.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/fwd.h"

namespace Engine::Extension::RendererModule::Assets {

Core::Runtime::CallbackResult ContextualizeVertexShader(Core::Runtime::ServiceTable *services, void *moduleState, Engine::Core::AssetManagement::AssetLoadingContext* outContext, size_t contextCount);
Core::Runtime::CallbackResult IndexVertexShader(Core::Runtime::ServiceTable *services, void *moduleState, Engine::Core::AssetManagement::AssetLoadingContext* inContext);

}