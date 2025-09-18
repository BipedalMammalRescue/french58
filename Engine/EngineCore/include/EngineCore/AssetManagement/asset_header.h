#pragma once

#include <cstddef>

namespace Engine::Core::AssetManagement {

struct AssetHeader
{
    // TODO: if we go with constexpr md5 route, this needs to be swapped out with std::array
    unsigned char Module[16];
    unsigned char Type[16];
    size_t DeclaredSize;
};

}