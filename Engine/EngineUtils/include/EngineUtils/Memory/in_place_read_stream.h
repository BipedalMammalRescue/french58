#pragma once

#include <cstddef>

namespace Engine::Utils::Memory {

class InPlaceReadStream 
{
  private:
    size_t m_Ptr = 0;
    size_t m_Length = 0;
    const unsigned char* m_Data = nullptr;
    
    void ReadCore(void* dest, size_t length);
    const unsigned char* ReadCore(size_t length);

  public:
    InPlaceReadStream(const unsigned char* data, size_t length) : m_Data(data), m_Length(length) {}
    
    template <typename T>
    T ReadCopy() 
    {
        T result;
        size_t resultLen = sizeof(T);
        ReadCore(&result, resultLen);
        return result;
    }
    
    template <typename T>
    T* ReadInPlace(size_t count = 1) 
    {
        return (T*)ReadCore(sizeof(T) * count);
    }
};

}