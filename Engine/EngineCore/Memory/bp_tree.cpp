#include "bp_tree.h"
#include "ErrorHandling/exceptions.h"
#include "high_integrity_allocator.h"
#include "Configuration/compile_time_flags.h"

#include <cstdint>
#include <ios>
#include <sstream>
#include <utility>

using namespace Engine::Core::Memory;

namespace Engine {
namespace Core {
namespace Memory {

struct Overflow
{
    IBpNode* Right = nullptr;
    uint64_t Key = 0;
    bool Valid = false;
};

struct Underflow 
{

};

class IBpNode
{
protected:
    friend class BpTree;
    uint64_t m_Keys[Configuration::B_PLUS_TREE_K_ARY - 1];
    void* m_Pointers[Configuration::B_PLUS_TREE_K_ARY];
    HighIntegrityAllocator* m_Allocator;
    int m_Fill = 0;

public:
    IBpNode(HighIntegrityAllocator* allocator)
        : m_Allocator(allocator)
    {
        // zero out memory since we use nullptr for some status indicator
        memset(m_Keys, 0, (Configuration::B_PLUS_TREE_K_ARY - 1) * sizeof(int));
        memset(m_Pointers, 0, Configuration::B_PLUS_TREE_K_ARY * sizeof(void*));
    }

    virtual Overflow TryInsert(uint64_t key, void* value) = 0;
    virtual bool TryGet(uint64_t key, void*& outValue) const = 0;
    virtual bool TryRemove(uint64_t key, void*& outValue) = 0;
    virtual ~IBpNode() = default;
    virtual void Print(std::stringstream& stream) const = 0;
};

class InternalNode : public IBpNode
{
public:
    Overflow TryInsert(uint64_t key, void* value) override;
    bool TryGet(uint64_t key, void*& value) const override;
    bool TryRemove(uint64_t key, void*& value) override;

    InternalNode(HighIntegrityAllocator* allocator)
        : IBpNode(allocator)
    {}

    ~InternalNode()
    {
        for (int i = 0; i <= m_Fill; i++)
        {
            if (m_Pointers[i] != nullptr)
            {
                IBpNode* pointer = (IBpNode*) m_Pointers[i];
                m_Allocator->Delete(pointer);
            }
        }
    }

    void Print(std::stringstream& stream) const override;
};

class LeafNode : public IBpNode
{
public:
    Overflow TryInsert(uint64_t key, void* value) override;
    bool TryGet(uint64_t key, void*& outValue) const override;
    bool TryRemove(uint64_t key, void*& outValue) override;

    LeafNode(HighIntegrityAllocator* allocator)
        : IBpNode(allocator)
    {}

    ~LeafNode() = default;

    void Print(std::stringstream& stream) const override;
};

}}}


void BpTree::Print(std::string& outString) const 
{ 
    std::stringstream stream;
    m_Root->Print(stream);
    outString.append(std::move(stream.str()));
}


BpTree::~BpTree() 
{ 
    if (m_Root != nullptr)
    {
        m_Allocator->Delete(m_Root); 
    }
}


Overflow InternalNode::TryInsert(uint64_t key, void* value) 
{
    // internal nodes don't directly add shit
    int targetPointer = 0;
    // if its greater than any existing key it'll just stop at target = m_Fill
    for (; targetPointer < m_Fill; targetPointer ++)
    {
        if (key <= m_Keys[targetPointer])
            break;
    }

    // pass the insertion down
    IBpNode* nextNode = (IBpNode*) m_Pointers[targetPointer];
    Overflow childOverflow = nextNode->TryInsert(key, value);

    // skip if the child node doesn't request an overflow
    if (!childOverflow.Valid)
        return childOverflow;

    // insert the item here
    if (m_Fill < Configuration::B_PLUS_TREE_K_ARY - 1)
    {
        // no overflow
        for (int i = m_Fill - 1; i >= targetPointer; i--) 
        {
            m_Keys[i + 1] = m_Keys[i];
            m_Pointers[i + 2] = m_Pointers[i + 1];
        }

        // fill in the vacated pointer-key-pointer set
        m_Keys[targetPointer] = childOverflow.Key;
        m_Pointers[targetPointer + 1] = childOverflow.Right;
        m_Fill++;
        return Overflow{};
    }
    else
    {
        // internal node overflow
        unsigned int branchLength = Configuration::B_PLUS_TREE_K_ARY / 2;

        // create an overflow buffer
        uint64_t overflowKeys[Configuration::B_PLUS_TREE_K_ARY];
        void* overflowPtr[Configuration::B_PLUS_TREE_K_ARY + 1];
        
        // populate overflow buffer (counting on compiler reordering them to make them fast)
        memcpy(overflowKeys, m_Keys, targetPointer * sizeof(uint64_t));
        overflowKeys[targetPointer] = childOverflow.Key;
        memcpy(overflowKeys + targetPointer + 1, m_Keys + targetPointer, (Configuration::B_PLUS_TREE_K_ARY - targetPointer - 1) * sizeof(uint64_t));
        memcpy(overflowPtr, m_Pointers, (targetPointer + 1) * sizeof(void*));
        overflowPtr[targetPointer + 1] = childOverflow.Right;
        memcpy(overflowPtr + targetPointer + 2, m_Pointers + targetPointer + 1, (Configuration::B_PLUS_TREE_K_ARY - targetPointer - 1) * sizeof(void*));

        // initialize a new sibling
        InternalNode* newSibling = m_Allocator->New<InternalNode>(m_Allocator);

        // split the overflow buffer
        memcpy(m_Keys, overflowKeys, branchLength * sizeof(uint64_t));
        memcpy(newSibling->m_Keys, overflowKeys + branchLength + 1, branchLength * sizeof(uint64_t));
        memcpy(m_Pointers, overflowPtr, (branchLength + 1) * sizeof(void*));
        memcpy(newSibling->m_Pointers, overflowPtr + branchLength + 1, (branchLength + 1) * sizeof(void*));

        // set lengths
        m_Fill = branchLength;
        newSibling->m_Fill = branchLength;

        // initialize a new overflow
        Overflow selfOverflow 
        {
            newSibling,
            overflowKeys[branchLength],
            true
        };

        return selfOverflow;
    }
}


Overflow LeafNode::TryInsert(uint64_t key, void* value) 
{
    // seek internally
    int targetPointer = 0;
    for (; targetPointer < m_Fill; targetPointer ++)
    {
        // override duplicate key
        if (key == m_Keys[targetPointer])
        {
            m_Pointers[targetPointer] = value;
            return Overflow{};
        }

        if (key < m_Keys[targetPointer])
            break;
    }

    // insert value
    if (m_Fill < Configuration::B_PLUS_TREE_K_ARY - 1) 
    {
        // no overflow
        for (int i = m_Fill - 1; i >= targetPointer; i--)
        {
            m_Keys[i + 1] = m_Keys[i];
            m_Pointers[i + 1] = m_Pointers[i];
        }

        m_Keys[targetPointer] = key;
        m_Pointers[targetPointer] = value;
        m_Fill ++;
        return Overflow{};
    }
    else
    {
        // leaf overflow
        unsigned int branchLength = Configuration::B_PLUS_TREE_K_ARY / 2;

        // create an overflow buffer
        uint64_t overflowKeys[Configuration::B_PLUS_TREE_K_ARY];
        void* overflowPtr[Configuration::B_PLUS_TREE_K_ARY + 1];
        
        // populate overflow buffer (counting on compiler reordering them to make them fast)
        memcpy(overflowKeys, m_Keys, targetPointer * sizeof(uint64_t));
        overflowKeys[targetPointer] = key;
        memcpy(overflowKeys + targetPointer + 1, m_Keys + targetPointer, (Configuration::B_PLUS_TREE_K_ARY - targetPointer - 1) * sizeof(uint64_t));
        memcpy(overflowPtr, m_Pointers, targetPointer * sizeof(void*));
        overflowPtr[targetPointer] = value;
        memcpy(overflowPtr + targetPointer + 1, m_Pointers + targetPointer + 1, (Configuration::B_PLUS_TREE_K_ARY - targetPointer - 1) * sizeof(void*));

        // initialize a new sibling
        LeafNode* newSibling = m_Allocator->New<LeafNode>(m_Allocator);

        // split the overflow buffer
        memcpy(m_Keys, overflowKeys, (branchLength + 1) * sizeof(uint64_t));
        memcpy(newSibling->m_Keys, overflowKeys + branchLength + 1, (branchLength) * sizeof(uint64_t));
        memcpy(m_Pointers, overflowPtr, (branchLength + 1) * sizeof(void*));
        memcpy(newSibling->m_Pointers, overflowPtr + branchLength + 1, (branchLength + 1) * sizeof(void*));
        m_Pointers[branchLength + 1] = newSibling;

        // set lengths
        m_Fill = branchLength + 1;
        newSibling->m_Fill = branchLength;

        // initialize a new overflow
        Overflow selfOverflow 
        {
            newSibling,
            overflowKeys[branchLength],
            true
        };

        return selfOverflow;
    }
}


bool InternalNode::TryGet(uint64_t key, void*& outValue) const
{
    // seek for pointer
    for (int i = 0; i < m_Fill; i ++)
    {
        if (key <= m_Keys[i])
        {
            if (m_Pointers[i] == nullptr)
                return false;

            return ((IBpNode*)m_Pointers[i])->TryGet(key, outValue);
        }
    }
    if (m_Pointers[m_Fill] == nullptr)
        return false;

    return ((IBpNode*)m_Pointers[m_Fill])->TryGet(key, outValue);
}


bool LeafNode::TryGet(uint64_t key, void*& outValue) const 
{
    for (int i = 0; i < m_Fill; i++)
    {
        if (key == m_Keys[i])
        {
            outValue = m_Pointers[i];
            return true;
        }
    }

    return false;
}


bool InternalNode::TryRemove(uint64_t key, void*& outValue)
{
    SE_THROW_NOT_IMPLEMENTED;
}


bool LeafNode::TryRemove(uint64_t key, void*& outValue)
{
    SE_THROW_NOT_IMPLEMENTED;
}


void Engine::Core::Memory::BpTree::Insert(uint64_t key, void *value) 
{
    if (m_Root == nullptr) 
    {
        m_Root = m_Allocator->New<LeafNode>(m_Allocator);
    }

    Overflow rootOverflow = m_Root->TryInsert(key, value);
    if (!rootOverflow.Valid)
        return;

    InternalNode *newRoot = m_Allocator->New<InternalNode>(m_Allocator);
    newRoot->m_Keys[0] = rootOverflow.Key;
    newRoot->m_Pointers[0] = m_Root;
    newRoot->m_Pointers[1] = rootOverflow.Right;
    newRoot->m_Fill = 1;
    m_Root = newRoot;
}


bool Engine::Core::Memory::BpTree::Remove(uint64_t key, void*& outValue) 
{
    return m_Root == nullptr ? false : m_Root->TryRemove(key, outValue);
}


bool Engine::Core::Memory::BpTree::TryGet(uint64_t key, void *&outValue) const
{
    if (m_Root == nullptr)
        return false;

    return m_Root->TryGet(key, outValue);
}


void Engine::Core::Memory::LeafNode::Print(std::stringstream& stream) const 
{
    stream << "leaf <" << std::hex << (unsigned long long)this << '>' << std::endl;

    for (int i = 0; i < m_Fill; i++) 
    {
        stream << m_Keys[i] << ":" << std::hex << (unsigned long long)m_Pointers[i] << " ";
    }

    stream << "->[" << std::hex << (unsigned long long)m_Pointers[m_Fill] << ']' << std::endl << std::endl;
}


void Engine::Core::Memory::InternalNode::Print(std::stringstream& stream) const 
{
    stream << "internal <" << std::hex << (unsigned long long)this << ">" << std::endl;
    for (int i = 0; i < m_Fill; i++) {
        stream << '[' << std::hex << (unsigned long long)m_Pointers[i] << "] " << m_Keys[i]
            << " ";
    }
    stream << '[' << std::hex << (unsigned long long)m_Pointers[m_Fill] << ']' << std::endl << std::endl;

    for (int i = 0; i <= m_Fill; i++) 
    {
        if (m_Pointers[i] != nullptr) 
        {
            ((IBpNode *)m_Pointers[i])->Print(stream);
        }
    }
}
