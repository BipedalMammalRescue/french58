#pragma once
#include <cstddef>

namespace Engine::Utils::Memory {

inline size_t AlignLength(size_t originalLength, size_t alignment)
{
    return originalLength + ((alignment - (originalLength % alignment)) % alignment);
}

} // namespace Engine::Utils::Memory