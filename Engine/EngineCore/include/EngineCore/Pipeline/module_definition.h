#pragma once

#include "asset_pipeline.h"

namespace Engine::Core::Pipeline {

struct AssetPipelineArray
{
    AssetPipeline *AssetsV = nullptr;
    size_t AssetsC = 0;
};

// includes everything defined in a module
struct ModuleDefinition
{
    // static definitions
    const char *Name = nullptr;
    AssetPipelineArray Assets;
};

} // namespace Engine::Core::Pipeline