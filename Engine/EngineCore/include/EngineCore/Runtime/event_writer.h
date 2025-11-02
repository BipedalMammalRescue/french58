#pragma once

#include "EngineCore/Runtime/event_stream.h"
#include "EngineCore/Runtime/event_manager.h"

namespace Engine::Core::Runtime 
{

class EventWriter
{
private:
    friend class EventManager;
    friend class GameLoop;

    const char* m_UserName = nullptr;
    EventStream m_Stream;

public:
    inline void Initialize()
    {
        m_Stream.ResetWriter();
    }

    template <typename TEvent>
    void WriteInputEvent(EventOwner<TEvent>* owner, TEvent eventData, int authorPath)
    {
        m_Stream.Write((void*)owner, &eventData, sizeof(TEvent), m_UserName, authorPath);
    }
};

}