#pragma once

#include "EngineCore/Runtime/fwd.h"
#include "EngineCore/Pipeline/asset_definition.h"

#include <array>
#include <md5.h>

namespace Engine::Core::Pipeline {

// includes everything defined in a module
struct ModuleDefinition
{
    // static definitions
    std::array<unsigned char, 16> Name;

    // module state
    void *(*Initialize)(Runtime::ServiceTable *services);
    void (*Dispose)(Runtime::ServiceTable *services, void *moduleState);

    // assets
    const AssetDefinition *Assets;
    size_t AssetsCount;

    // TODO: components
};

} // namespace Engine::Core::Pipeline