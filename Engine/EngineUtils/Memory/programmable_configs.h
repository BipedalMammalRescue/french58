#pragma once

#include <cstddef>

namespace Engine::Utils::Memory {

template <size_t K = 1, size_t B = 1> class LinearGrowth
{
  public:
    static size_t NextAllocationSize(size_t x)
    {
        return x > 0 ? K : B;
    }
};

}