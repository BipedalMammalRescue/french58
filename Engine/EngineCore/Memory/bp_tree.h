#pragma once

namespace Engine {
namespace Core {
namespace Memory {

class HighIntegrityAllocator;
struct BpLink;
struct BpNode;

// convention: left pointer is lte, right pointer is gt
class BpTree
{
private:
    HighIntegrityAllocator* m_Allocator;
    BpNode* m_Head = nullptr;
    unsigned int m_Depth = 0;

    BpNode* LeafLookup(unsigned long long key);


    void ResolveInteralOverflow(BpLink* overflowLink, BpNode* targetParent, BpNode* leftChild, BpNode* rightChild);

public:
    // insert a value;
    // returns whether it's inserted (false if key already exists)
    bool Insert(unsigned long long key, void* value);

    struct KeyValuePair 
    {
        unsigned long long Key;
        void* Value;
    };

    // remove a value based on exact match of key and writes the removed value to outValue;
    // returns whether value is remvoed
    bool Remove(unsigned long long key, void*& outValue);

    // look up a singular element with exact key and writes it to outValue;
    // returns whether value is found
    bool Lookup(unsigned long long key, void*& outValue) const;

    // look up a range of elements, the acceptor will be invoked on each of them syncronously in order;
    // returns number of elements iterated
    template <typename TFunk>
    unsigned int RangeLookup(unsigned long long start, unsigned long long end, TFunk&& acceptor) const
    {
        // TODO: implement range look up as two look ups in different directions, then iterate through the resulting elements
    }
};

}}}