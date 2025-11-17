#pragma once

#include "EngineCore/Runtime/event_stream.h"
#include "EngineCore/Runtime/event_manager.h"

namespace Engine::Core::Runtime 
{

struct EventWriterCheckpoint
{
    size_t PrevLength;
};

class EventWriter
{
private:
    friend class EventManager;
    friend class GameLoop;

    const char* m_UserName = nullptr;

    std::vector<unsigned char> m_Data;

    void Write(void* owner, const void* data, size_t length, const char* authorName, int authorPath);

    inline void Initialize()
    {
        // clear the data stream to just a init block
        m_Data.resize(sizeof(EventHeader));
        EventHeader initialBlockHeader {
            nullptr,
            0,
            nullptr,
            0
        };
        memcpy(m_Data.data(), &initialBlockHeader, sizeof(EventHeader));
    }

public:
    template <typename TEvent>
    void WriteInputEvent(const EventOwner<TEvent>* owner, TEvent eventData, int authorPath)
    {
        Write((void*)owner, &eventData, sizeof(TEvent), m_UserName, authorPath);
    }

    inline EventStream OpenReadStream()
    {
        return EventStream(&m_Data);
    }

    inline bool HasEvents() const
    {
        return m_Data.size() > sizeof(EventHeader);
    }

    inline EventWriterCheckpoint CreateCheckpoint() const 
    {
        return { m_Data.size() };
    }

    inline void Rollback(EventWriterCheckpoint checkpoint)
    {
        m_Data.resize(checkpoint.PrevLength);
    }
};

}