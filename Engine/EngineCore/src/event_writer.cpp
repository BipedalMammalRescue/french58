#include "EngineCore/Runtime/event_writer.h"

using namespace Engine::Core::Runtime;

void EventWriter::Write(void* owner, const void* data, size_t length, const char* authorName, int authorPath)
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