#include "EngineCore/Runtime/index_queue.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/transient_allocator.h"

using namespace Engine::Core::Runtime;

IndexQueue::IndexQueue(TransientBufferId bufferId, AssetManagement::AsyncAssetEvent* buffer, size_t count, const Logging::Logger& logger, Runtime::ServiceTable* services)
    : m_BufferId(bufferId),
    m_Buffer(buffer),
    m_Count(count),
    m_PendingPosition(0),
    m_Next(nullptr),
    m_Logger(logger),
    m_Services(services)
{
}

Engine::Core::Runtime::CallbackResult IndexQueue::Flush()
{
    for (; m_PendingPosition < m_Count; m_PendingPosition++)
    {
        auto& currentEvent = m_Buffer[m_PendingPosition];

        if (!m_Buffer[m_PendingPosition].IsAvailable())
            return Runtime::CallbackSuccess();

        // skip unindexable objects
        if (m_Buffer[m_PendingPosition].IsBroken())
        {
            m_Logger.Information("Asset {} became unindexable and is skipped.", currentEvent.GetContext()->AssetId);
            continue;
        }

        // index this object
        m_Logger.Information("Indexing asset {} {}.", currentEvent.GetDefinition()->Name.DisplayName, currentEvent.GetContext()->AssetId);
        Runtime::CallbackResult result = currentEvent.GetDefinition()->Index(m_Services, currentEvent.GetModuleState(), currentEvent.GetContext());
        if (result.has_value())
            return result;

        if (currentEvent.GetContext()->Buffer.Type == AssetManagement::LoadBufferType::TransientBuffer)
        {
            m_Services->TransientAllocator->Return( currentEvent.GetContext()->Buffer.Location.TransientBufferId);
        }

        currentEvent.MakeAvailable();
    }

    if (m_Next == nullptr)
        return CallbackSuccess();
    
    return m_Next->Flush();
}