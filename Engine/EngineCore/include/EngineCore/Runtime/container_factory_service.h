#pragma once

#include "EngineCore/Containers/Uniform/sorted_array.h"
#include "EngineCore/Containers/container_allocation_strategy.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Logging/logger_service.h"
#include <cstdlib>

namespace Engine::Core::Runtime {

class ContainerFactoryService : public Containers::IContainerAllocationStrategy
{
private:
    Logging::Logger m_Logger;
    size_t m_BufferCounter;

public:
    ContainerFactoryService(Logging::LoggerService* loggerService) 
        : m_Logger(loggerService->CreateLogger("ContainerFactoryService")),
        m_BufferCounter(0)
    {}

    template <typename T>
    Containers::Uniform::TrivialSortedArray<T> CreateSortedArray(size_t initialCapacity)
    {
        return Containers::Uniform::TrivialSortedArray<T>(this, initialCapacity);
    }

    template <typename TElement, typename TCompare>
    Containers::Uniform::SortedArray<TElement, TCompare> CreateSortedArray(size_t initialCapacity)
    {
        return Containers::Uniform::SortedArray<TElement, TCompare>(this, initialCapacity);
    }

    void* ToClientBuffer(void* buffer)
    {
        return ((char*)buffer) + sizeof(size_t);
    }

    void* ToBufferHeader(void* buffer)
    {
        return ((char*)buffer) - sizeof(size_t);
    }

public:
    void* Allocate(size_t minimumCapacity)
    {
        size_t actualAllocationSize = minimumCapacity + sizeof(size_t);
        void* buffer = malloc(actualAllocationSize);

        // allocate a buffer with a helper id buffer
        *static_cast<size_t*>(buffer) = m_BufferCounter;
        m_Logger.Information("Allocated buffer #{} for {} bytes.", m_BufferCounter, minimumCapacity);
        m_BufferCounter ++;

        // calculate the client buffer
        return ToClientBuffer(buffer);
    }

    void* Reallocate(void* oldBuffer, size_t newSize)
    {
        // calculate the header contained buffer
        void* buffer = ToBufferHeader(oldBuffer);
        size_t bufferId = *static_cast<size_t*>(buffer);

        // reallocate
        void* newBuffer = realloc(buffer, newSize + sizeof(size_t));
        m_Logger.Information("Reallocated buffer #{} for {} bytes.", bufferId, newSize);

        // calculate client buffer
        return ToClientBuffer(newBuffer);
    }

    void Free(void* clientBuffer)
    {
        void* buffer = ToBufferHeader(clientBuffer);
        size_t bufferId = *static_cast<size_t*>(buffer);
        free(buffer);
        m_Logger.Information("Buffer #{} freed.", bufferId);
    }
};

}