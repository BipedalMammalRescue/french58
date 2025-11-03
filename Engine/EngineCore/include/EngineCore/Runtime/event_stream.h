#pragma once

#include <cstring>
#include <vector>

namespace Engine::Core::Runtime 
{

struct EventHeader
{
    void* Owner;
    size_t Length;

    const char* AuthorName;
    int AuthorPath;
};

class EventStream
{
private:
    friend class EventWriter;

    const std::vector<unsigned char>* m_Data;
    size_t m_Cursor = 0;

    EventStream(const std::vector<unsigned char>* data) : m_Data(data) {}

public:
    bool MoveNext();
    EventHeader GetCurrentHeader() const;
    const void* GetCurrentData() const;
};

}