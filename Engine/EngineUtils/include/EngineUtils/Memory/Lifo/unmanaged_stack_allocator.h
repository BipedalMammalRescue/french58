#pragma once

#include "EngineUtils/Memory/programmable_configs.h"

#include <cstddef>

namespace Engine::Utils::Memory::Lifo {

// no header in buffer (requires client code to remember allocated data), self-growing, LIFO allocator
class UnmanagedStackAllocator
{
  private:
    size_t m_Capacity;
    size_t m_NextFree;
    unsigned char *m_Buffer;

    size_t ReserveCore(size_t nextCapacity);
    size_t AllocateCore(size_t targetSize);
    size_t DeallocateCore(size_t startingBuffer);

  public:
    UnmanagedStackAllocator(size_t initialCapacity);

    ~UnmanagedStackAllocator();

    template <typename TGrowth = QuadraticGrowth<>> size_t Reserve(size_t totalSize)
    {
        size_t allocationSize = m_Capacity;

        while (allocationSize < totalSize)
        {
            allocationSize = TGrowth::NextAllocationSize(allocationSize);
        }

        return ReserveCore(allocationSize);
    }

    template <typename TGrowth = QuadraticGrowth<>> size_t Allocate(size_t targetSize)
    {
        Reserve<TGrowth>(m_NextFree + targetSize);
        return AllocateCore(targetSize);
    }

    size_t Deallocate(size_t startingBuffer)
    {
        return DeallocateCore(startingBuffer);
    }

    inline unsigned char *GetPointer(size_t offset)
    {
        return m_Buffer + offset;
    }

#ifdef DEBUG_OR_TEST
    inline size_t DebugGetCapacity()
    {
        return m_Capacity;
    }
    inline size_t DebugGetOffset()
    {
        return m_NextFree;
    }
#endif
};

} // namespace Engine::Utils::Memory::Lifo