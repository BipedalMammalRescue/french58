#pragma once

#include "EngineCore/Runtime/asset_manager.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/service_table.h"

namespace Engine::Extension::OrcaRendererModule::Assets {

enum class ShaderStage : unsigned char
{
    Vertex,
    Fragment
};

Core::Runtime::CallbackResult ContextualizeShader(
    Core::Runtime::ServiceTable *services, void *moduleState, Core::AssetManagement::AssetLoadingContext* outContext, size_t contextCount);

Core::Runtime::CallbackResult IndexShader(
    Core::Runtime::ServiceTable *services, void *moduleState, Core::AssetManagement::AssetLoadingContext* inContext);

}