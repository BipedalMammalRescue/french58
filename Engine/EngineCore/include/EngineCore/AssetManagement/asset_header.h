#pragma once

#include <cstddef>

namespace Engine::Core::AssetManagement {

struct AssetHeader
{
    unsigned char Module[16];
    unsigned char Type[16];
    size_t DeclaredSize;
};

}