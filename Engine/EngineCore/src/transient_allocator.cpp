#include "EngineCore/Runtime/transient_allocator.h"
#include "EngineCore/Logging/logger_service.h"
#include <cstdlib>

using namespace Engine::Core::Runtime;

TransientAllocator::TransientAllocator(Engine::Core::Logging::LoggerService* loggerService)
    :m_Logger(loggerService->CreateLogger("TransientAllocator"))
{
    m_Buffers.resize(16);
}

TransientBufferId Engine::Core::Runtime::TransientAllocator::CreateBufferGroup(size_t totalSize, int childCount)
{
    int targetIndex;
    for (targetIndex = 0; targetIndex < m_Buffers.size() && m_Buffers[targetIndex].Buffer != nullptr; targetIndex++) {}

    if (targetIndex == m_Buffers.size())
    {
        m_Buffers.push_back({nullptr, 0});
    }

    m_Buffers[targetIndex].Buffer = malloc(totalSize);
    m_Buffers[targetIndex].ChildCount = childCount;

    m_Logger.Information("Allocating buffer group, index: {}, total size: {}, child count: {}.", targetIndex, totalSize, childCount);

    return { targetIndex, 0 };
}

void Engine::Core::Runtime::TransientAllocator::Return(TransientBufferId id)
{
    int parent = id.Parent;

    if (parent < 0 || parent >= m_Buffers.size())
    {
        m_Logger.Warning("Trying to return transient buffer out of allocated range! Available: 0-{}, got: {}.", m_Buffers.size() - 1, parent);
        return;
    }

    if (m_Buffers[parent].Buffer == nullptr)
    {
        m_Logger.Warning("Trying to return transient buffer already deallocated! Index: {}.", parent);
        return;
    }

    m_Buffers[parent].ChildCount --;
    if (m_Buffers[parent].ChildCount <= 0)
    {
        free(m_Buffers[parent].Buffer);
        m_Buffers[parent].Buffer = nullptr;
        m_Logger.Information("Transient buffer #{} deallocated.", parent);
    }
}

void *Engine::Core::Runtime::TransientAllocator::GetBuffer(TransientBufferId id) 
{
    int parent = id.Parent;

    if (parent < 0 || parent >= m_Buffers.size())
    {
        m_Logger.Warning("Trying to access transient buffer out of allocated range! Available: 0-{}, got: {}.", m_Buffers.size() - 1, parent);
        return nullptr;
    }

    if (m_Buffers[parent].Buffer == nullptr)
    {
        m_Logger.Warning("Trying to access transient buffer already deallocated! Index: {}.", parent);
        return nullptr;
    }

    unsigned char* buffer = static_cast<unsigned char*>(m_Buffers[parent].Buffer);
    return buffer + id.Child;
}
