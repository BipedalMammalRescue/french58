#pragma once

#include "EngineCore/Logging/logger.h"
#include <cstddef>
#include <vector>

namespace Engine::Core::Logging {
    class LoggerService;
}

namespace Engine::Core::Runtime {

struct TransientBufferId
{
    int Parent;
    int Child;
};

struct AllocatedBuffer
{
    void* Buffer = nullptr;
    int ChildCount = 0;
};

class TransientAllocator
{
private:
    Logging::Logger m_Logger;
    std::vector<AllocatedBuffer> m_Buffers;

public:
    TransientAllocator(Logging::LoggerService* loggerService);

    // returns the id of the first buffer
    TransientBufferId CreateBufferGroup(size_t totalSize, int childCount);
    
    void Return(TransientBufferId id);
    
    void *GetBuffer(TransientBufferId id);
};

}