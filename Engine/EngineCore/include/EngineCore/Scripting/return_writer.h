#pragma once

#include <cstddef>

namespace Engine::Core::Scripting {

class IReturnWriter
{
protected:
    virtual void WriteCore(const void* data, size_t size) = 0;

public:
    template <typename T>
    void Write(const T* data)
    {
        WriteCore(data, sizeof(T));
    }
};

}