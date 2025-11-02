#pragma once

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

    std::vector<unsigned char> m_Data;
    size_t m_Cursor = 0;

    void Write(void* owner, const void* data, size_t length, const char* authorName, int authorPath);

public:
    inline void ResetWriter() 
    {
        m_Data.clear();
    }

    inline void ResetReader()
    {
        m_Cursor = 0;
    }

    bool MoveNext();
    EventHeader GetCurrentHeader() const;
    const void* GetCurrentData() const;
};

}