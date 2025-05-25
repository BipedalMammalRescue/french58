#pragma once

#include "loaded_asset.h"
#include <cstdint>
#include <vector>

namespace Engine::Core::AssetManagement {

class AssetManager
{
  private:
    // asset manager stores its data in a continuous storage space, the need to grow the space aligns with using a vecotr
    std::vector<unsigned char> m_Data;

  public:
    LoadedAsset CreateAsset(const size_t length, const uint64_t id);
    LoadedAsset GetAsset(const uint64_t id);
    
    // delete one or many assets, and optionally ensure a new totalLength in internal storage
    void DeleteAssets(const uint64_t* assets, const size_t newLength = 0);
};

}