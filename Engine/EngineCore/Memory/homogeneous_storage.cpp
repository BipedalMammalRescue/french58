#include "homogeneous_storage.h"
#include <algorithm>

// creates a chain segment data structure, and a buffer immediately after it

using namespace Engine::Core::Memory;

HomogeneousStorage::Segment* HomogeneousStorage::CreateNewSegment(unsigned int count, size_t payloadSize)
{
    // create a buffer whose size is the required amount of nodes PLUS a segment header
    size_t nodeSize = std::max(sizeof(FreeNode), payloadSize);
    size_t chainSize = nodeSize * count;
    char* newBuffer = new char[chainSize + sizeof(Segment)];

    // initialize the first address into a header
    Segment* newSegment = new (newBuffer) Segment;
    newSegment->Count = count;

    ResetSegment(newSegment);

    // header is at the beginning of the new memory range
    return newSegment;
}


// it does change internal state, not a const function

void HomogeneousStorage::ResetSegment(Segment* newSegment)
{
    char* newBuffer = (char*)newSegment;
    size_t nodeSize = std::max(sizeof(FreeNode), m_payloadSize);

    // initialize first n-1 nodes
    char* nodesBegin = newBuffer + sizeof(Segment);
    for (unsigned int i = 0; i < newSegment->Count - 1; i++)
    {
        FreeNode* currentNode = (FreeNode*)(nodesBegin + nodeSize * i);
        FreeNode* nextNode = (FreeNode*)(nodesBegin + nodeSize * i + nodeSize);
        currentNode->Next = nextNode;
    }

    // initialize the last node (idk, it's good to avoid having lots of if-statements?)
    FreeNode* lastNode = (FreeNode*)(nodesBegin + nodeSize * newSegment->Count - nodeSize);
    lastNode->Next = nullptr;
}


HomogeneousStorage::~HomogeneousStorage()
{
    // increment to the next segment then delete the current
    // delete is called on the BufferChainSegment* 
    Segment* nextSeg = m_headSegment;
    while (nextSeg != nullptr)
    {
        Segment* currentSeg = nextSeg;
        nextSeg = nextSeg->NextSegment;
        delete[] currentSeg;
    }
}


void* HomogeneousStorage::Take()
{
    // initialize the first segment
    if (m_headSegment == nullptr)
    {
        // allocate a new segment and initialize the chain on top level
        m_headSegment = CreateNewSegment(m_initialCount, m_payloadSize);
        m_tailSegment = m_headSegment;

        // the new segment always supplies its first free node at the beginning of its buffer
        m_headNode = m_headSegment->GetFirstNode();
    }

    // extend the list
    if (m_headNode == nullptr)
    {
        Segment* newSegment = CreateNewSegment(m_tailSegment->Count * 2, m_payloadSize);
        m_tailSegment->NextSegment = newSegment;
        m_tailSegment = newSegment;
        m_headNode = m_tailSegment->GetFirstNode();
    }

    // increment head and return the previous head
    void* result = (void*)m_headNode;
    m_headNode = m_headNode->Next;
    return result;
}


void Engine::Core::Memory::HomogeneousStorage::Put(void* pointer)
{
    FreeNode* header = (FreeNode*)pointer;
    header->Next = m_headNode;
    m_headNode = header;
}


void HomogeneousStorage::Reset()
{
    if (m_headSegment == nullptr)
        return;

    for (Segment* segment = m_headSegment; segment != nullptr; segment = segment->NextSegment)
    {
        ResetSegment(segment);
    }

    m_headNode = m_headSegment->GetFirstNode();
}
