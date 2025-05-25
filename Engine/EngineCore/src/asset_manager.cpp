#include "EngineCore/AssetManagement/asset_manager.h"
#include "EngineUtils/ErrorHandling/exceptions.h"

using namespace Engine::Core::AssetManagement;

LoadedAsset Engine::Core::AssetManagement::AssetManager::CreateAsset(size_t length, uint64_t id)
{
    // TODO: this logic is dependent on the memory management mechanism
    SE_THROW_NOT_IMPLEMENTED;
}

LoadedAsset Engine::Core::AssetManagement::AssetManager::GetAsset(const uint64_t id)
{
    SE_THROW_NOT_IMPLEMENTED;
}
