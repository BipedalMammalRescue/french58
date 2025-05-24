#pragma once

#include <stddef.h>

namespace Engine::Core::AssetManagement {

struct LoadedAsset
{
    void *Buffer;
    size_t Length;
};

} // namespace Engine::Core::AssetManagement