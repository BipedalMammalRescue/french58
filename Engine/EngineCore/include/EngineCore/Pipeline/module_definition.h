#pragma once

#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Runtime/fwd.h"
#include "EngineCore/Pipeline/asset_definition.h"
#include "EngineCore/Pipeline/engine_callback.h"
#include "EngineCore/Pipeline/component_definition.h"

#include <md5.h>

namespace Engine::Core::Pipeline {

// includes everything defined in a module
struct ModuleDefinition
{
    // static definitions
    HashId Name;

    // module state
    void *(*Initialize)(Runtime::ServiceTable *services);
    void (*Dispose)(Runtime::ServiceTable *services, void *moduleState);

    // assets
    const AssetDefinition* Assets;
    size_t AssetsCount;

    // event handler
    const EngineCallback* Callbacks;
    size_t CallbackCount;

    // components
    const ComponentDefinition* Components;
    size_t ComponentCount;
};

} // namespace Engine::Core::Pipeline