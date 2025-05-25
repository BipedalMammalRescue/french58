#include "in_place_read_stream.h"

#include <ErrorHandling/exceptions.h>
#include <cstring>

void Engine::Utils::Memory::InPlaceReadStream::ReadCore(void *dest, size_t length)
{
    // bound check
    if (m_Ptr + length > m_Length)
        SE_THROW_READ_OVERFLOW_EXCEPTION;

    std::memcpy(dest, (m_Data + m_Ptr), length);
    m_Ptr += length;
}

const unsigned char *Engine::Utils::Memory::InPlaceReadStream::ReadCore(size_t length)
{
    // bound check
    if (m_Ptr + length > m_Length)
        SE_THROW_READ_OVERFLOW_EXCEPTION;
        
    const unsigned char* result = m_Data + m_Ptr;
    m_Ptr += length;
    
    return result;
}
