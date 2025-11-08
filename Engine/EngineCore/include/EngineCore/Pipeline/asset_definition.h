#pragma once

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
};

}