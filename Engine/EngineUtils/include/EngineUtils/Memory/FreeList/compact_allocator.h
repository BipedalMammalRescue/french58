#pragma once

#include "EngineUtils/ErrorHandling/exceptions.h"
#include "EngineUtils/Memory/programmable_configs.h"

#include <cstddef>
#include <cstdint>

namespace Engine::Utils::Memory::FreeList {

// Compact allocator allocates everything in comapct (no padding between allocated elements).
// Due to the simpler logic, the only size limitation is 8bytes lower bound (again, sub-word allocation should not use
// free list allocator)
template <typename T, size_t TItemsPerBlock = 64, typename TGrowthFactor = LinearGrowth<>> class CompactAllocator
{
  private:
    union Item {
        T Data;
        Item *NextFree;
    };

    struct Block
    {
        uint64_t Bitmask;
        Item Items[TItemsPerBlock];
    };

    Block **m_Blocks = nullptr;
    size_t m_BlockCount = 0;
    size_t m_FreeListHead = 1;

  public:
    CompactAllocator()
    {
        SE_THROW_ALGORITHMIC_EXCEPTION;
    }
};

} // namespace Engine::Utils::Memory::FreeList
