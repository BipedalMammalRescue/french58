#pragma once

#include "EngineCore/AssetManagement/asset_loading_context.h"
#include "EngineCore/Pipeline/name_pair.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/fwd.h"

namespace Engine::Core::Pipeline {

class IAssetEnumerator;

struct AssetDefinition
{
    NamePair Name;

    // new asset system
    Runtime::CallbackResult (*Contextualize)(Runtime::ServiceTable *services, void *moduleState, AssetManagement::AssetLoadingContext* outContext, size_t contextCount);
    Runtime::CallbackResult (*Index)(Runtime::ServiceTable *services, void *moduleState, AssetManagement::AssetLoadingContext* inContext);
};

}