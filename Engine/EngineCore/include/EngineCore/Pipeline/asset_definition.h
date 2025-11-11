#pragma once

#include "EngineCore/AssetManagement/asset_loading_context.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/name_pair.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/fwd.h"

#include <cstddef>

namespace Engine::Core::Pipeline {

class IAssetEnumerator;

struct AssetDefinition
{
    NamePair Name;

    // load function needs to support batch operations natively
    Runtime::CallbackResult (*Load)(IAssetEnumerator *inputStreams, Runtime::ServiceTable *services, void *moduleState);
    Runtime::CallbackResult (*Unload)(HashId *ids, size_t count, Runtime::ServiceTable *services,
                   void *moduleState);

    // new asset system
    Runtime::CallbackResult (*Contextualize)(Runtime::ServiceTable *services, void *moduleState, AssetManagement::AssetLoadingContext& outContext);
    Runtime::CallbackResult (*Index)(Runtime::ServiceTable *services, void *moduleState, AssetManagement::AssetLoadingContext inContext);
    Runtime::CallbackResult (*Destroy)(Runtime::ServiceTable* services, void* moduleState, AssetManagement::AssetLoadingContext loadContext);
};

}