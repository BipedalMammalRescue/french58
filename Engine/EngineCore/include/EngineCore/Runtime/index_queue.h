#pragma once

#include "EngineCore/AssetManagement/async_io_event.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/service_table.h"
#include "EngineCore/Runtime/transient_allocator.h"

namespace Engine::Core::Runtime {

class IndexQueue
{
private:
    TransientBufferId m_BufferId;

    AssetManagement::AsyncAssetEvent* m_Buffer;
    size_t m_Count;
    size_t m_PendingPosition;
    
    IndexQueue* m_Next;

    Logging::Logger m_Logger;
    Runtime::ServiceTable* m_Services;

public:
    IndexQueue(TransientBufferId bufferId, AssetManagement::AsyncAssetEvent* buffer, size_t count, const Logging::Logger& logger, Runtime::ServiceTable* services);

    inline TransientBufferId GetBufferId()
    {
        return m_BufferId;
    }

    inline IndexQueue* GetNext() 
    {
        return m_Next;
    }

    inline void AddTail(IndexQueue* tail)
    {
        if (m_Next == nullptr)
        {
            m_Next = tail;
        }
        else 
        {
            m_Next->AddTail(tail);
        }
    }

    inline bool IsCompleted() const 
    {
        return m_PendingPosition >= m_Count;
    }

    Runtime::CallbackResult Flush();
};

}