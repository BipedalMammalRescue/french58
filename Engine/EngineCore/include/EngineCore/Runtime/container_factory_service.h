#pragma once

#include "EngineCore/Containers/Uniform/sorted_array.h"
#include "EngineCore/Containers/container_allocation_strategy.h"
#include <cstdlib>

namespace Engine::Core::Runtime {

class ContainerFactoryService : public Containers::IContainerAllocationStrategy
{
public:
    template <typename T>
    Containers::Uniform::TrivialSortedArray<T> CreateSortedArray(size_t initialCapacity)
    {
        return Containers::Uniform::TrivialSortedArray<T>(this, initialCapacity);
    }

    template <typename TElement, typename TCompare>
    Containers::Uniform::SortedArray<TElement, TCompare> CreateSortedArray(size_t initialCapacity)
    {
        return Containers::Uniform::SortedArray<TElement, TCompare>(this, initialCapacity);
    }

public:
    void* Allocate(size_t minimumCapacity)
    {
        return malloc(minimumCapacity);
    }

    void* Reallocate(void* oldBuffer, size_t newSize)
    {
        return realloc(oldBuffer, newSize);
    }

    void Free(void* buffer)
    {
        free(buffer);
    }
};

}