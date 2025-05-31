#pragma once

#include <cstddef>

namespace Engine::Utils::Memory {

template <size_t K = 2> class QuadraticGrowth
{
  public:
    static size_t NextAllocationSizeIncremental(size_t x)
    {
        return NextAllocationSize(x) - x;
    }

    static size_t NextAllocationSize(size_t x)
    {
        return K * x;
    }
};

}