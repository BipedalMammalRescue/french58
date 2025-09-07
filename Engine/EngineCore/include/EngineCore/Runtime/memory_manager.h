#pragma once

#include <EngineUtils/Memory/Lifo/unmanaged_stack_allocator.h>

namespace Engine::Core::Runtime {

class MemoryManager
{
  private:
    Utils::Memory::Lifo::UnmanagedStackAllocator m_RootAllocator;

  public:
    MemoryManager(size_t rootAllocatorInitialSize) : m_RootAllocator(rootAllocatorInitialSize) {};

    size_t Allocate(size_t size);
    unsigned char *GetPointer(size_t offset);
    void Deallocate(size_t offset);
};

} // namespace Engine::Core::Runtime