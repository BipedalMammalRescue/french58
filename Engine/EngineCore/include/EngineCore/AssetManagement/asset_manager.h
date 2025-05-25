#pragma once

#include "loaded_asset.h"
#include <cstdint>

namespace Engine::Core::AssetManagement {

class AssetManager
{
  public:
    LoadedAsset CreateAsset(const size_t length, const uint64_t id);
    LoadedAsset GetAsset(const uint64_t id);
};

}