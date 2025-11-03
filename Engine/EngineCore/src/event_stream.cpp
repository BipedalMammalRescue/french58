#include "EngineCore/Runtime/event_stream.h"
#include <cstring>

using namespace Engine::Core::Runtime;

bool EventStream::MoveNext()
{
    // increment cursor
    EventHeader header = GetCurrentHeader();
    m_Cursor += sizeof(EventHeader) + header.Length;

    // align to longest alignment
    m_Cursor -= 1;
    m_Cursor = m_Cursor - (m_Cursor % sizeof(size_t)) + sizeof(size_t);
    return m_Cursor < m_Data->size();
}

EventHeader EventStream::GetCurrentHeader() const
{
    return *(EventHeader*)(m_Data->data() + m_Cursor);
}

const void* EventStream::GetCurrentData() const
{
    return m_Data->data() + m_Cursor + sizeof(EventHeader);
}