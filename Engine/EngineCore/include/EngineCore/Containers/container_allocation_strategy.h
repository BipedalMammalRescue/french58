#pragma once

#include <cstddef>
namespace Engine::Core::Containers {

class IContainerAllocationStrategy
{
public:
    virtual void* Allocate(size_t minimumCapacity) = 0;
    virtual void* Reallocate(void* oldBuffer, size_t newSize) = 0;
    virtual void Free(void* buffer) = 0;
};

}