#include "EngineCore/Runtime/memory_manager.h"

using namespace Engine::Core::Runtime;

size_t MemoryManager::Allocate(size_t size)
{
    return m_RootAllocator.Allocate(size);
}

unsigned char *MemoryManager::GetPointer(size_t offset)
{
    return m_RootAllocator.GetPointer(offset);
}

void MemoryManager::Deallocate(size_t size)
{
    m_RootAllocator.Deallocate(size);
}