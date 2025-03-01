#include "high_integrity_allocator.h"
#include "Configuration/compile_time_flags.h"
#include "ErrorHandling/exceptions.h"
#include "Utils/shorthand_functions.h"
#include "homogeneous_storage.h"
#include <cstring>

using namespace Engine::Core::Memory;
using namespace Engine::Core::Utils;

// a small header encoding the lower n bytes (configured by Configuration::ALLOCATOR_SIZE_HINT_WIDTH) for the size of
// the buffer after it
struct SizeHintHeader
{
    unsigned char SizeHint[Engine::Core::Configuration::ALLOCATOR_SIZE_HINT_WIDTH];
};

void *HighIntegrityAllocator::AllocateCore(size_t size)
{
    HomogeneousStorage *targetStore = nullptr;

    auto foundStorage = m_BufferTable.find(size);
    if (foundStorage != m_BufferTable.end())
    {
        targetStore = foundStorage->second;
    }
    else
    {
        targetStore = new HomogeneousStorage(size + sizeof(void *), m_InitialBufferItemCount);
        m_BufferTable.insert(std::make_pair(size, targetStore));
    }

    SizeHintHeader *paddedResult = (SizeHintHeader *)targetStore->Take();
    memcpy(paddedResult->SizeHint, &size, Configuration::ALLOCATOR_SIZE_HINT_WIDTH);
    return Utils::SkipHeader<SizeHintHeader>(paddedResult);
}

Engine::Core::Memory::HighIntegrityAllocator::HighIntegrityAllocator(unsigned int initialSize)
    : m_InitialBufferItemCount(initialSize)
{
}

Engine::Core::Memory::HighIntegrityAllocator::~HighIntegrityAllocator()
{
    for (auto &pair : m_BufferTable)
    {
        delete pair.second;
    }
}

void Engine::Core::Memory::HighIntegrityAllocator::Free(void *pointer)
{
    SizeHintHeader *header = GetHeader<SizeHintHeader>(pointer);
    unsigned int key = 0;
    memcpy(&key, header->SizeHint, Configuration::ALLOCATOR_SIZE_HINT_WIDTH);
    auto targetStore = m_BufferTable.find(key);
    if (targetStore == m_BufferTable.end())
        SE_THROW_ALLOCATOR_EXCEPTION;

    targetStore->second->Put(header);
}
