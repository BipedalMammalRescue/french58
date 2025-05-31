#include "EngineUtils/Memory/Lifo/unmanaged_stack_allocator.h"

#include <cstdint>
#include <stdlib.h>

size_t Engine::Utils::Memory::Lifo::UnmanagedStackAllocator::ReserveCore(size_t nextCapacity)
{
    if (nextCapacity <= m_Capacity)
        return m_Capacity;

    m_Buffer = (m_Buffer == nullptr) ? (unsigned char *)malloc(nextCapacity)
                                     : (unsigned char *)realloc(m_Buffer, nextCapacity);
    m_Capacity = nextCapacity;
    return nextCapacity;
}

size_t Engine::Utils::Memory::Lifo::UnmanagedStackAllocator::AllocateCore(size_t targetSize)
{
    if (m_NextFree + targetSize > m_Capacity)
        return SIZE_MAX;

    size_t result = m_NextFree;
    m_NextFree += targetSize;

    return result;
}

size_t Engine::Utils::Memory::Lifo::UnmanagedStackAllocator::DeallocateCore(size_t startingBuffer)
{
    if (startingBuffer >= m_NextFree)
        return SIZE_MAX;

    m_NextFree = startingBuffer;
    return m_NextFree;
}

Engine::Utils::Memory::Lifo::UnmanagedStackAllocator::~UnmanagedStackAllocator()
{
    free(m_Buffer);
}

Engine::Utils::Memory::Lifo::UnmanagedStackAllocator::UnmanagedStackAllocator(size_t initialCapacity)
    : m_Capacity(initialCapacity), m_NextFree(0), m_Buffer((unsigned char *)malloc(initialCapacity))
{
}