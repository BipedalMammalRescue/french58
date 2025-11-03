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

    // these callbacks are never called in parallel and are thus granted full access to every resource in the engine
    const SynchronousCallback* SynchronousCallbacks;
    size_t SynchronousCallbackCount;

    // event callbacks are called to handle *INPUT EVENT*, engine guarantees that ever callback itself is only called by one thread at a time, but between callbacks can be scheduled arbitrarily
    const EventCallback* EventCallbacks;
    size_t EventCallbackCount;

    // components
    const ComponentDefinition* Components;
    size_t ComponentCount;
};

} // namespace Engine::Core::Pipeline