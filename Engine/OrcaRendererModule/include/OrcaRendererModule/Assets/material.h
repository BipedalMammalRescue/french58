#pragma once

#include "EngineCore/AssetManagement/asset_loading_context.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/service_table.h"
#include "OrcaRendererModule/Runtime/renderer_resource.h"
#include <stddef.h>

namespace Engine::Extension::OrcaRendererModule::Assets {

// Materials is an explicit instantiation of a shader. It provides a page in the resource database.
// TODO: the implementation of this class is not correct.

struct Material
{
    // material points to a shader by reference
    size_t ShaderRef;
    size_t ResourceCount;

    Runtime::NamedRendererResource *GetResources()
    {
        return (Runtime::NamedRendererResource *)(this + 1);
    }
};
Engine::Core::Runtime::CallbackResult ContextualizeMaterial(
    Engine::Core::Runtime::ServiceTable *services, void *moduleState,
    Engine::Core::AssetManagement::AssetLoadingContext *outContext, size_t contextCount);

Engine::Core::Runtime::CallbackResult IndexFragmentMaterial(
    Engine::Core::Runtime::ServiceTable *services, void *moduleState,
    Engine::Core::AssetManagement::AssetLoadingContext *inContext);

} // namespace Engine::Extension::OrcaRendererModule::Assets