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

public:
    ContainerFactoryService(Logging::LoggerService* loggerService) : m_Logger(loggerService->CreateLogger("ContainerFactoryService")) {}

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

public:
    void* Allocate(size_t minimumCapacity)
    {
        m_Logger.Information("Allocating for {} bytes.", minimumCapacity);
        return malloc(minimumCapacity);
    }

    void* Reallocate(void* oldBuffer, size_t newSize)
    {
        m_Logger.Information("Reallocating for {} bytes.", newSize);
        return realloc(oldBuffer, newSize);
    }

    void Free(void* buffer)
    {
        m_Logger.Information("Buffer freed.");
        free(buffer);
    }
};

}