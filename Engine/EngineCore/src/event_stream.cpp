#include "EngineCore/Runtime/event_stream.h"
#include <cstring>

using namespace Engine::Core::Runtime;

void EventStream::Write(void* owner, const void* data, size_t length, const char* authorName, int authorPath)
{
    // calculate total length written
    size_t totalLength = sizeof(EventHeader) + length;
    totalLength -= 1;
    totalLength = totalLength - (totalLength % sizeof(size_t)) + sizeof(size_t);

    // increment data position
    size_t cursor = m_Data.size();
    m_Data.resize(m_Data.size() + totalLength);

    // insert data
    EventHeader header { owner, length, authorName, authorPath };
    memcpy(m_Data.data() + cursor, &header, sizeof(header));
    memcpy(m_Data.data() + cursor + sizeof(header), data, length);
}

bool EventStream::MoveNext()
{
    // increment cursor
    EventHeader header = GetCurrentHeader();
    m_Cursor += sizeof(EventHeader) + header.Length;

    // align to longest alignment
    m_Cursor -= 1;
    m_Cursor = m_Cursor - (m_Cursor % sizeof(size_t)) + sizeof(size_t);
    return m_Cursor < m_Data.size();
}

EventHeader EventStream::GetCurrentHeader() const
{
    return *(EventHeader*)(m_Data.data() + m_Cursor);
}

const void* EventStream::GetCurrentData() const
{
    return m_Data.data() + m_Cursor + sizeof(EventHeader);
}