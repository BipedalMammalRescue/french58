#pragma once

#include "EngineCore/Containers/container_allocation_strategy.h"
#include "SDL3/SDL_stdinc.h"

#include <cstddef>
#include <cstdlib>

namespace Engine::Core::Containers::Uniform {

template <typename T> struct TrivialComparer
{
    static int Compare(const T *a, const T *b)
    {
        if (*a < *b)
            return -1;
        if (*a > *b)
            return 1;
        return 0;
    }
};

template <typename TKey, typename TValue> struct AnnotatedNode
{
    TKey Key;
    TValue Value;
};

template <typename TKey, typename TValue> struct AnnotatedNodeComparer
{
    static int Compare(const AnnotatedNode<TKey, TValue> *a, const AnnotatedNode<TKey, TValue> *b)
    {
        if (a->Key < b->Key)
            return -1;
        if (a->Key > b->Key)
            return 1;
        return 0;
    }
};

// Always continuous, always sorted in ascending order.
template <typename T, typename TCompare> class SortedArray
{
private:
    static int CompareCore(const void *a, const void *b)
    {
        const T *elementA = static_cast<const T *>(a);
        const T *elementB = static_cast<const T *>(b);

        return TCompare::Compare(elementA, elementB);
    }

    template <typename TCustomCompare> static int CompareCoreCustom(const void *a, const void *b)
    {
        const T *elementA = static_cast<const T *>(a);
        const T *elementB = static_cast<const T *>(b);

        return TCustomCompare::Compare(elementA, elementB);
    }

    IContainerAllocationStrategy *m_Allocator;
    T *m_Storage;
    size_t m_Size;
    size_t m_Capacity;

public:
    SortedArray(IContainerAllocationStrategy *allocator, size_t initialCapacity)
        : m_Allocator(allocator), m_Storage(nullptr), m_Size(0), m_Capacity(initialCapacity)
    {
        if (initialCapacity > 0)
        {
            m_Storage = static_cast<T *>(allocator->Allocate(initialCapacity * sizeof(T)));
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
        m_Storage = (T *)m_Allocator->Reallocate((void *)m_Storage, (m_Size + count) * sizeof(T));
        m_Capacity = m_Size + count;
    }

    void ReserveTotal(size_t count)
    {
        if (count <= m_Capacity)
            return;
        m_Storage = (T *)m_Allocator->Reallocate((void *)m_Storage, count * sizeof(T));
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

    bool RangeCheck(size_t position) const
    {
        return position >= 0 && position < m_Size;
    }

    // Insert a singular element, if the key is not unqiue a duplicate is inserted.
    void Insert(const T &element)
    {
        ReserveExtra(1);

        if (m_Size == 0)
        {
            m_Storage[0] = element;
            m_Size++;
            return;
        }

        // roll a custom inexact binary search
        size_t candidate = FindLowerBound(element);

        // move every element larger than or equal to the candidate rightward
        for (size_t movee = m_Size - 1; movee >= candidate && RangeCheck(movee); movee--)
        {
            m_Storage[movee + 1] = m_Storage[movee];
        }

        // insert the element at candidate
        m_Storage[candidate] = element;
        m_Size++;
    }

    // Insert a singular element, if the key is not unique the insertion is dropped.
    bool TryInsert(const T &element)
    {
        ReserveExtra(1);

        if (m_Size == 0)
        {
            m_Storage[0] = element;
            m_Size++;
            return true;
        }

        // roll a custom inexact binary search
        size_t candidate = FindLowerBound(element);

        // drop the operation if the new element is not unique
        if (candidate < m_Size && TCompare::Compare(&element, &m_Storage[candidate]) == 0)
            return false;

        // move every element larger than or equal to the candidate rightward
        for (size_t movee = m_Size - 1; movee >= candidate && RangeCheck(movee); movee--)
        {
            m_Storage[movee + 1] = m_Storage[movee];
        }

        // insert the element at candidate
        m_Storage[candidate] = element;
        m_Size++;

        return true;
    }

    // Ensures the existence of an item equivalent to element and get it, insert element as is if an equivalent is not
    // found.
    T *GetOrAdd(const T &element)
    {
        if (m_Size == 0)
        {
            m_Storage[0] = element;
            m_Size++;
            return &m_Storage[0];
        }

        // roll a custom inexact binary search
        size_t candidate = FindLowerBound(element);

        // insert the new item if we dont' find it already
        if (!(candidate < m_Size && TCompare::Compare(&element, &m_Storage[candidate]) == 0))
        {
            ReserveExtra(1);

            // move every element larger than or equal to the candidate rightward
            for (size_t movee = m_Size - 1; movee >= candidate && RangeCheck(movee); movee--)
            {
                m_Storage[movee + 1] = m_Storage[movee];
            }

            // insert the element at candidate
            m_Storage[candidate] = element;
            m_Size++;
        }

        return &m_Storage[candidate];
    }

    // Insert a singular element, if the key is not unique the original copy is overwritten
    void Replace(const T &element)
    {
        ReserveExtra(1);

        if (m_Size == 0)
        {
            m_Storage[0] = element;
            m_Size++;
            return;
        }

        // roll a custom inexact binary search
        size_t candidate = FindLowerBound(element);

        // replace existing entry if the target already exists
        if (candidate < m_Size && TCompare::Compare(&element, &m_Storage[candidate]) == 0)
        {
            m_Storage[candidate] = element;
            return;
        }

        // move every element larger than or equal to the candidate rightward
        for (size_t movee = m_Size - 1; movee >= candidate && RangeCheck(movee); movee--)
        {
            m_Storage[movee + 1] = m_Storage[movee];
        }

        // insert the element at candidate
        m_Storage[candidate] = element;
        m_Size++;
    }

    // find the first element to be bigger than argument
    size_t FindLowerBound(const T &element)
    {
        // weird edge case
        if (m_Size == 0)
            return 0;

        size_t candidate = m_Size / 2;
        switch (TCompare::Compare(&element, &m_Storage[candidate]))
        {
        case 0:
            return candidate;
        case 1:
            for (candidate += 1; candidate < m_Size; candidate++)
            {
                switch (TCompare::Compare(&element, &m_Storage[candidate]))
                {
                case 0:
                case -1:
                    return candidate;
                default:
                    break;
                }
            }
            return m_Size;
        case -1:
            for (candidate -= 1; candidate >= 0; candidate--)
            {
                switch (TCompare::Compare(&element, &m_Storage[candidate]))
                {
                case 0:
                case 1:
                    return candidate + 1;
                default:
                    break;
                }
            }
            return 0;
        // not gonna happen
        default:
            return 0;
        }
    }

    // Allocate and asign all elements at once, then do a full buffer sort.
    void InsertRange(const T *elements, size_t count)
    {
        ReserveExtra(count);

        for (size_t i = 0; i < count; i++)
        {
            m_Storage[m_Size] = elements[i];
            m_Size++;
        }

        if (m_Size <= 1)
            return;

        SDL_qsort(m_Storage, m_Size, sizeof(T), CompareCore);
    }

    // Allocate and asign all elements at once, then do a full buffer sort; this version allows user to insert elements
    // from arbitrary sources.
    template <typename TUserData>
    void InsertRange(size_t count, TUserData *userdata, void (*writer)(T *, size_t, TUserData *))
    {
        ReserveExtra(count);

        writer(&m_Storage[m_Size], count, userdata);

        m_Size += count;
        SDL_qsort(m_Storage, m_Size, sizeof(T), CompareCore);
    }

    T *PtrAt(size_t index)
    {
        if (index >= m_Size)
            return nullptr;

        return &m_Storage[index];
    }

    const T *PtrAt(size_t index) const
    {
        if (index >= m_Size)
            return nullptr;

        return &m_Storage[index];
    }

    // EXACT SEARCH
    size_t Search(const T &key)
    {
        void *foundAddress = SDL_bsearch(&key, m_Storage, m_Size, sizeof(T), CompareCore);
        if (foundAddress == nullptr)
            return m_Size;

        return static_cast<T *>(foundAddress) - static_cast<T *>(m_Storage);
    }

    template <typename TCustomCompare> size_t CustomSearch(const T *key)
    {
        void *foundAddress = SDL_bsearch(key, m_Storage, m_Size, sizeof(T), CompareCoreCustom<TCustomCompare>);
        if (foundAddress == nullptr)
            return m_Size;

        return static_cast<T *>(foundAddress) - static_cast<T *>(m_Storage);
    }

    template <typename TCustomCompare> bool CustomContains(const T &key)
    {
        return CustomSearch<TCustomCompare>(key) != m_Size;
    }
};

template <typename T> using TrivialSortedArray = SortedArray<T, TrivialComparer<T>>;

template <typename TKey, typename TValue>
using AnnotationSortedArray = SortedArray<AnnotatedNode<TKey, TValue>, AnnotatedNodeComparer<TKey, TValue>>;

} // namespace Engine::Core::Containers::Uniform