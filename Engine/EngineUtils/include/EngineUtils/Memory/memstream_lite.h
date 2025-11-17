#pragma once

#include <cstddef>

namespace Engine::Utils::Memory {
    
struct MemStreamLite
{
    void* Buffer;
    size_t Cursor;

    template <typename T>
    T Read()
    {
        T result = *(T*)(static_cast<unsigned char*>(Buffer) + Cursor);
        Cursor += sizeof(T);
        return result;
    }

    void Seek(size_t offset)
    {
        Cursor = offset;
    }

    size_t GetPosition() const 
    {
        return Cursor;
    }
};

}