#pragma once

#include <cstddef>
#include <cstdlib>
namespace Engine::Core::Runtime {

class HeapAllocator
{
    // NOTE: temporary implementation
public:
    void* Allocate(size_t size)
    {
        return malloc(size);   
    }

    template <typename T>
    T* Allocator(size_t count)
    {
        return Allocate(count * sizeof(T));
    }

    void Deallocate(void* buffer)
    {
        free(buffer);
    }

    void* Realloc(void* buffer, size_t newSize)
    {
        return realloc(buffer, newSize);
    }

    template <typename T>
    T* Realloc(T* buffer, size_t count)
    {
        return Realloc((void*)buffer, count * sizeof(T));
    }
};

}