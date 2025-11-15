#pragma once

#include "EngineCore/Containers/container_allocation_strategy.h"
#include "SDL3/SDL_stdinc.h"
#include <cstddef>
#include <cstdlib>

namespace Engine::Core::Containers::Uniform {

template <typename T>
struct TrivialComparer
{
    static int Compare(const T* a, const T* b)
    {
        if (*a < *b)
            return -1;
        if (*a > *b)
            return 1;
        return 0;
    }
};

// Always continuous, always sorted in ascending order.
template <typename T, typename TCompare>
class SortedArray
{
private:
    static int CompareCore(const void* a, const void* b)
    {
        const T* elementA = static_cast<const T*>(a);
        const T* elementB = static_cast<const T*>(b);
        
        return TCompare::Compare(elementA, elementB);
    }

    template <typename TCustomCompare>
    static int CompareCoreCustom(const void* a, const void* b)
    {
        const T* elementA = static_cast<const T*>(a);
        const T* elementB = static_cast<const T*>(b);
        
        return TCustomCompare::Compare(elementA, elementB);
    }

    IContainerAllocationStrategy* m_Allocator;
    void* m_Storage;
    size_t m_Size;
    size_t m_Capacity;

    inline T* TypedStorage() 
    {
        return static_cast<T*>(m_Storage);
    }

public:
    SortedArray(IContainerAllocationStrategy* allocator, size_t initialCapacity)
        : m_Allocator(allocator), m_Storage(nullptr), m_Size(0), m_Capacity(initialCapacity)
    {
        if (initialCapacity > 0)
        {
            m_Storage = allocator->Allocate(initialCapacity * sizeof(T));
        }
    }

    void Destroy()
    {
        m_Allocator->Free(m_Storage);
        m_Storage = nullptr;
    }

    void ReserveExtra(size_t count) 
    {
        if (m_Size + count <= m_Capacity)
            return;
        m_Storage = realloc(m_Storage, (m_Size + count) * sizeof(T));
        m_Capacity = m_Size + count;
    }

    void ReserveTotal(size_t count) 
    {
        if (count <= m_Capacity)
            return;
        m_Storage = realloc(m_Storage, count * sizeof(T));
        m_Capacity = count;
    }

    size_t GetCount() const
    {
        return m_Size;
    }

    size_t GetCapacity() const
    {
        return m_Capacity;
    }

    // Insert a singular element; ideally not doing a full sort.
    void Insert(const T& element)
    {
        // TODO: need to run a binary search and insert + move
        InsertRange(&element, 1);
    }

    // Allocate and asign all elements at once, then do a full buffer sort.
    void InsertRange(const T* elements, size_t count) 
    {
        ReserveExtra(count);

        for (size_t i = 0; i < count; i++)
        {
            TypedStorage()[m_Size] = elements[i];
            m_Size ++;
        }

        if (m_Size <= 1)
            return;

        SDL_qsort(m_Storage, m_Size, sizeof(T), CompareCore);
    }

    // Allocate and asign all elements at once, then do a full buffer sort; this version allows user to insert elements from arbitrary sources.
    template <typename TUserData>
    void InsertRange(size_t count, TUserData* userdata, void(*writer)(T*, size_t, TUserData*))
    {
        ReserveExtra(count);

        writer(&TypedStorage()[m_Size], count, userdata);

        m_Size += count;
        SDL_qsort(m_Storage, m_Size, sizeof(T), CompareCore);
    }

    T* PtrAt(size_t index)
    {
        if (index >= m_Size)
            return nullptr;

        return &TypedStorage()[index];
    }

    const T* PtrAt(size_t index) const
    {
        if (index >= m_Size)
            return nullptr;

        return &TypedStorage()[index];
    }

    size_t Search(const T* key)
    {
        void* foundAddress = SDL_bsearch(key, m_Storage, m_Size, sizeof(T), CompareCore);
        if (foundAddress == nullptr)
            return m_Size;

        return static_cast<T*>(foundAddress) - static_cast<T*>(m_Storage);
    }

    template <typename TCustomCompare>
    size_t CustomSearch(const T* key)
    {
        void* foundAddress = SDL_bsearch(key, m_Storage, m_Size, sizeof(T), CompareCoreCustom<TCustomCompare>);
        if (foundAddress == nullptr)
            return m_Size;

        return static_cast<T*>(foundAddress) - static_cast<T*>(m_Storage);
    }
};

template <typename T>
using TrivialSortedArray = SortedArray<T, TrivialComparer<T>>;

}