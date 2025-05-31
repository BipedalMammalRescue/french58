#include "EngineCore/AssetManagement/asset_manager.h"
#include "EngineCore/Runtime/memory_manager.h"

#include <EngineUtils/ErrorHandling/exceptions.h>
#include <EngineUtils/Memory/alignment_calc.h>

using namespace Engine::Core::AssetManagement;

struct AssetIndexNode
{
    uint64_t Key;
    LoadedAsset Value;
};

LoadedAsset AssetManager::CreateAsset(size_t length, uint64_t id)
{
    SE_THROW_NOT_IMPLEMENTED;
}

LoadedAsset AssetManager::GetAsset(const uint64_t id)
{
    SE_THROW_NOT_IMPLEMENTED;
}

void AssetManager::ReorgnizeAssets(LoadedAsset* assetsV, size_t assetsC)
{
    // pop old buffer
    m_MemoryManager->Deallocate(m_Buffer);

    // calculate new index size
    size_t indexSize = assetsC * sizeof(AssetIndexNode);

    // calculate new buffer size
    size_t newSize = 0;
    for (size_t i = 0; i < assetsC; i++)
    {
        // align size to 8 bytes
        assetsV[i].Length = Utils::Memory::AlignLength(assetsV[i].Length, sizeof(size_t));

        newSize += assetsV[i].Length;
        newSize += sizeof(AssetIndexNode);
    }

    SE_THROW_NOT_IMPLEMENTED;
}
