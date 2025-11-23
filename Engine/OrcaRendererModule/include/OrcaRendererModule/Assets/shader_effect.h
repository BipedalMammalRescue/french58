#pragma once

#include "EngineCore/AssetManagement/asset_loading_context.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/service_table.h"
namespace Engine::Extension::OrcaRendererModule::Assets {

Core::Runtime::CallbackResult ContextualizeShaderEffect(
    Core::Runtime::ServiceTable *services, void *moduleState, Core::AssetManagement::AssetLoadingContext* outContext, size_t contextCount);

Core::Runtime::CallbackResult IndexShaderEffect(
    Core::Runtime::ServiceTable *services, void *moduleState, Core::AssetManagement::AssetLoadingContext* inContext);

}