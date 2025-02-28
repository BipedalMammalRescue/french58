#pragma once

#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>


namespace Engine {
namespace Core {
namespace Memory {

class HighIntegrityAllocator;
struct BpLink;
struct BpNode;

class IBpNode;

class BpTree
{
private:
    IBpNode* m_Root = nullptr;
    HighIntegrityAllocator* m_Allocator;

public:
    void Insert(uint64_t key, void *value);

    bool Remove(uint64_t key, void*& outValue);

    bool TryGet(uint64_t key, void *&outValue) const;

    void Print(std::string& outString) const;

    BpTree(HighIntegrityAllocator* allocator)
        : m_Allocator(allocator)
    {}

    ~BpTree();
};

}}}