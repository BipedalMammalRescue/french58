#pragma once

#include "loaded_asset.h"
#include <cstdint>
#include <vector>

namespace Engine::Core::Runtime {

class MemoryManager;

}

namespace Engine::Core::AssetManagement {

class AssetManager
{
  private:
    Runtime::MemoryManager *m_MemoryManager;
    size_t m_Buffer;

  public:
    AssetManager(Runtime::MemoryManager *memoryManager) : m_MemoryManager(memoryManager)
    {
    }

    LoadedAsset CreateAsset(const size_t length, const uint64_t id);
    LoadedAsset GetAsset(const uint64_t id);

    // re-initializes the asset storage to a new roster; MUST be called after cleaning up scene buffer
    void ReorgnizeAssets(LoadedAsset* assetsV, size_t assetsC);
};

} // namespace Engine::Core::AssetManagement