#pragma once
#include "Utils/shorthand_functions.h"
#include <cstddef>

namespace Engine {
namespace Core {
namespace Memory {

// single-linked list of free buffers
class HomogeneousStorage
{
  private:
    // Meatadata header that shares its memory usage with the payload, since it's only active after the payload had been
    // freed
    struct FreeNode
    {
        FreeNode *Next = nullptr;
    };

    // A segment of a chain of buffers.
    struct Segment
    {
        Segment *NextSegment = nullptr;
        unsigned int Count = 0;

        FreeNode *GetFirstNode() const
        {
            return (FreeNode *)Utils::SkipHeader<Segment>(this);
        }
    };

    // creates a chain segment data structure, and a buffer immediately after it
    Segment *CreateNewSegment(unsigned int count);

    // state starts here:

    Segment *m_headSegment = nullptr;
    Segment *m_tailSegment = nullptr;

    FreeNode *m_headNode = nullptr;

    size_t m_payloadSize = 0;
    unsigned int m_initialCount = 0;

    // it does change internal state, not a const function
    void ResetSegment(Segment *newSegment);

  public:
    HomogeneousStorage(size_t payloadSize, unsigned int initialCount)
        : m_payloadSize(payloadSize), m_initialCount(initialCount)
    {
    }
    ~HomogeneousStorage();

    void *Take();

    void Put(void *pointer);

    void Reset();
};

} // namespace Memory
} // namespace Core
} // namespace Engine
