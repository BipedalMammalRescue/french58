#pragma once

#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Runtime/fwd.h"

#include <array>
#include <cstddef>

namespace Engine::Core::Pipeline {

class AssetEnumerable;

struct AssetDefinition
{
    std::array<unsigned char, 16> Name;

    // load function needs to support batch operations natively
    void (*Load)(AssetEnumerable *inputStreams, Runtime::ServiceTable *services, void *moduleState);
    void (*Unload)(HashId *ids, size_t count, Runtime::ServiceTable *services,
                   void *moduleState);
};

}