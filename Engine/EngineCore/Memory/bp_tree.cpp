#include "bp_tree.h"
#include "Configuration/compile_time_flags.h"
#include "ErrorHandling/exceptions.h"
#include "high_integrity_allocator.h"

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
    IBpNode *Right = nullptr;
    uint64_t Key = 0;
    bool Valid = false;
};

enum class UnderflowResolution
{
    MERGE,
    REDISTRIBUTE,
    NO_UNDERFLOW,
    NO_DELETION,
    UNRESOLVED_LEAF,
    UNRESOLVED_INTERNAL,
    TOTAL
};

// reports underflow resolution back to the calling parent
struct Underflow
{
    // pointer related to the resolution: for MERGE this is the node that's merged into, for REDISTRIBUTE this is the
    // node that redistributed with
    void *Pointer = nullptr;
    UnderflowResolution Resolution = UnderflowResolution::TOTAL;
};

class IBpNode
{
  protected:
    friend class BpTree;
    friend class InternalNode;
    friend class LeafNode;
    uint64_t m_Keys[Configuration::B_PLUS_TREE_K_ARY - 1];
    void *m_Pointers[Configuration::B_PLUS_TREE_K_ARY];
    HighIntegrityAllocator *m_Allocator;
    int m_Fill = 0;

  public:
    IBpNode(HighIntegrityAllocator *allocator) : m_Allocator(allocator)
    {
        // zero out memory since we use nullptr for some status indicator
        memset(m_Keys, 0, (Configuration::B_PLUS_TREE_K_ARY - 1) * sizeof(int));
        memset(m_Pointers, 0, Configuration::B_PLUS_TREE_K_ARY * sizeof(void *));
    }

    virtual Overflow TryInsert(uint64_t key, void *value) = 0;
    virtual bool TryGet(uint64_t key, void *&outValue) const = 0;
    virtual Underflow TryRemove(uint64_t key, void *&outValue, IBpNode *leftSibling, IBpNode *rightSibling) = 0;
    virtual ~IBpNode() = default;
    virtual void Print(std::stringstream &stream) const = 0;
};

class InternalNode : public IBpNode
{
  public:
    Overflow TryInsert(uint64_t key, void *value) override;
    bool TryGet(uint64_t key, void *&value) const override;
    Underflow TryRemove(uint64_t key, void *&value, IBpNode *leftSibling, IBpNode *rightSibling) override;

    InternalNode(HighIntegrityAllocator *allocator) : IBpNode(allocator)
    {
    }

    ~InternalNode()
    {
        for (int i = 0; i <= m_Fill; i++)
        {
            IBpNode *pointer = (IBpNode *)m_Pointers[i];
            m_Allocator->Delete(pointer);
        }
    }

    void Print(std::stringstream &stream) const override;
};

class LeafNode : public IBpNode
{
  public:
    Overflow TryInsert(uint64_t key, void *value) override;
    bool TryGet(uint64_t key, void *&outValue) const override;
    Underflow TryRemove(uint64_t key, void *&outValue, IBpNode *leftSibling, IBpNode *rightSibling) override;

    inline void Link(LeafNode *rightSibling)
    {
        m_Pointers[m_Fill] = rightSibling;
    }

    LeafNode(HighIntegrityAllocator *allocator) : IBpNode(allocator)
    {
    }

    ~LeafNode() = default;

    void Print(std::stringstream &stream) const override;
};

} // namespace Memory
} // namespace Core
} // namespace Engine

void BpTree::Print(std::string &outString) const
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

Overflow InternalNode::TryInsert(uint64_t key, void *value)
{
    // internal nodes don't directly add shit
    int targetPointer = 0;
    // if its greater than any existing key it'll just stop at target = m_Fill
    for (; targetPointer < m_Fill; targetPointer++)
    {
        if (key <= m_Keys[targetPointer])
            break;
    }

    // pass the insertion down
    IBpNode *nextNode = (IBpNode *)m_Pointers[targetPointer];
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
        void *overflowPtr[Configuration::B_PLUS_TREE_K_ARY + 1];

        // populate overflow buffer (counting on compiler reordering them to make them fast)
        memcpy(overflowKeys, m_Keys, targetPointer * sizeof(uint64_t));
        overflowKeys[targetPointer] = childOverflow.Key;
        memcpy(overflowKeys + targetPointer + 1, m_Keys + targetPointer,
               (Configuration::B_PLUS_TREE_K_ARY - targetPointer - 1) * sizeof(uint64_t));
        memcpy(overflowPtr, m_Pointers, (targetPointer + 1) * sizeof(void *));
        overflowPtr[targetPointer + 1] = childOverflow.Right;
        memcpy(overflowPtr + targetPointer + 2, m_Pointers + targetPointer + 1,
               (Configuration::B_PLUS_TREE_K_ARY - targetPointer - 1) * sizeof(void *));

        // initialize a new sibling
        InternalNode *newSibling = m_Allocator->New<InternalNode>(m_Allocator);

        // split the overflow buffer
        memcpy(m_Keys, overflowKeys, branchLength * sizeof(uint64_t));
        memcpy(newSibling->m_Keys, overflowKeys + branchLength + 1, branchLength * sizeof(uint64_t));
        memcpy(m_Pointers, overflowPtr, (branchLength + 1) * sizeof(void *));
        memcpy(newSibling->m_Pointers, overflowPtr + branchLength + 1, (branchLength + 1) * sizeof(void *));

        // set lengths
        m_Fill = branchLength;
        newSibling->m_Fill = branchLength;

        // initialize a new overflow
        Overflow selfOverflow{newSibling, overflowKeys[branchLength], true};

        return selfOverflow;
    }
}

Overflow LeafNode::TryInsert(uint64_t key, void *value)
{
    // seek internally
    int targetPointer = 0;
    for (; targetPointer < m_Fill; targetPointer++)
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
        m_Fill++;
        return Overflow{};
    }
    else
    {
        // leaf overflow
        unsigned int branchLength = Configuration::B_PLUS_TREE_K_ARY / 2;

        // create an overflow buffer
        uint64_t overflowKeys[Configuration::B_PLUS_TREE_K_ARY];
        void *overflowPtr[Configuration::B_PLUS_TREE_K_ARY + 1];

        // populate overflow buffer (counting on compiler reordering them to make them fast)
        memcpy(overflowKeys, m_Keys, targetPointer * sizeof(uint64_t));
        overflowKeys[targetPointer] = key;
        memcpy(overflowKeys + targetPointer + 1, m_Keys + targetPointer,
               (Configuration::B_PLUS_TREE_K_ARY - targetPointer - 1) * sizeof(uint64_t));
        memcpy(overflowPtr, m_Pointers, targetPointer * sizeof(void *));
        overflowPtr[targetPointer] = value;
        memcpy(overflowPtr + targetPointer + 1, m_Pointers + targetPointer + 1,
               (Configuration::B_PLUS_TREE_K_ARY - targetPointer - 1) * sizeof(void *));

        // initialize a new sibling
        LeafNode *newSibling = m_Allocator->New<LeafNode>(m_Allocator);

        // split the overflow buffer
        memcpy(m_Keys, overflowKeys, (branchLength + 1) * sizeof(uint64_t));
        memcpy(newSibling->m_Keys, overflowKeys + branchLength + 1, (branchLength) * sizeof(uint64_t));
        memcpy(m_Pointers, overflowPtr, (branchLength + 1) * sizeof(void *));
        memcpy(newSibling->m_Pointers, overflowPtr + branchLength + 1, (branchLength + 1) * sizeof(void *));
        m_Pointers[branchLength + 1] = newSibling;

        // set lengths
        m_Fill = branchLength + 1;
        newSibling->m_Fill = branchLength;

        // initialize a new overflow
        Overflow selfOverflow{newSibling, overflowKeys[branchLength], true};

        return selfOverflow;
    }
}

bool InternalNode::TryGet(uint64_t key, void *&outValue) const
{
    // seek for pointer
    for (int i = 0; i < m_Fill; i++)
    {
        if (key <= m_Keys[i])
        {
            if (m_Pointers[i] == nullptr)
                return false;

            return ((IBpNode *)m_Pointers[i])->TryGet(key, outValue);
        }
    }
    if (m_Pointers[m_Fill] == nullptr)
        return false;

    return ((IBpNode *)m_Pointers[m_Fill])->TryGet(key, outValue);
}

bool LeafNode::TryGet(uint64_t key, void *&outValue) const
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

Underflow InternalNode::TryRemove(uint64_t key, void *&outValue, IBpNode *leftSibling, IBpNode *rightSibling)
{
    // seek the entry
    int targetPointer = 0;
    for (; targetPointer < m_Fill; targetPointer++)
    {
        if (m_Keys[targetPointer] <= key)
            break;
    }

    // run deletion from child
    IBpNode *targetChild = (IBpNode *)m_Pointers[targetPointer];
    IBpNode *leftChildSibling = targetPointer == 0 ? nullptr : (IBpNode *)m_Pointers[targetPointer - 1];
    IBpNode *rightChildSibling = targetPointer == m_Fill ? nullptr : (IBpNode *)m_Pointers[targetPointer + 1];
    Underflow childUnderflow = targetChild->TryRemove(key, outValue, leftChildSibling, rightChildSibling);
    if (childUnderflow.Resolution == UnderflowResolution::NO_DELETION)
        return childUnderflow;

    SE_THROW_NOT_IMPLEMENTED;
}

Underflow LeafNode::TryRemove(uint64_t key, void *&outValue, IBpNode *leftSibling, IBpNode *rightSibling)
{
    // TODO: first thing: remove an entry and move everything towards left
    // seek the entry
    int targetPointer = 0;
    for (; targetPointer < m_Fill; targetPointer++)
    {
        if (m_Keys[targetPointer] == key)
            break;
    }
    if (targetPointer >= m_Fill)
        return Underflow{nullptr, UnderflowResolution::NO_DELETION};

    outValue = m_Pointers[m_Fill];

    // override the entry
    for (int i = targetPointer; i < m_Fill - 1; i++)
    {
        m_Keys[i] = m_Keys[i + 1];
        m_Pointers[i] = m_Pointers[i + 1];
    }
    m_Pointers[m_Fill - 1] = m_Pointers[m_Fill];

    // set new length
    m_Fill--;

    // determine if there's underflow
    if (m_Fill > 1)
        return Underflow{nullptr, UnderflowResolution::NO_UNDERFLOW};

    LeafNode *leftSiblingReal = static_cast<LeafNode *>(leftSibling);
    LeafNode *rightSiblingReal = static_cast<LeafNode *>(rightSibling);

    // try to merge into a sibling, if both are full then redistribute with one of them
    // TODO:
    if (leftSibling != nullptr && leftSibling->m_Fill < Configuration::B_PLUS_TREE_K_ARY - 1)
    {
        // in case of vacate sibling, insert the content of the remining key to that node; return value can be safely
        // discarded because we know the sibling is a leaf node AND that it can never overflow
        leftSiblingReal->TryInsert(m_Keys[0], m_Pointers[0]);
        leftSiblingReal->Link(rightSiblingReal);
        return Underflow{leftSiblingReal, UnderflowResolution::MERGE};
    }
    else if (rightSibling != nullptr && rightSibling->m_Fill < Configuration::B_PLUS_TREE_K_ARY - 1)
    {
        // same procedure as above, except this time check whether the left sibling is valid
        rightSiblingReal->TryInsert(m_Keys[0], m_Pointers[0]);
        if (leftSibling != nullptr)
        {
            leftSiblingReal->Link(rightSiblingReal);
        }
        return Underflow{rightSiblingReal, UnderflowResolution::MERGE};
    }
    else
    {
        unsigned int branchLength = Configuration::B_PLUS_TREE_K_ARY / 2;

        // in case neither of the siblings can be merged, try to redistribute with one of them
        if (leftSibling != nullptr)
        {
            // move the valid entry in self K/2 slots backwards
            m_Keys[branchLength] = m_Keys[0];
            m_Pointers[branchLength] = m_Pointers[0];
            m_Fill = branchLength + 1;

            // to avoid runtime cost we copy the tail from sibling here
            memcpy(m_Keys, leftSiblingReal->m_Keys + branchLength, branchLength * sizeof(uint64_t));
            memcpy(m_Pointers, leftSiblingReal->m_Pointers + branchLength, branchLength * sizeof(void *));
            leftSiblingReal->m_Fill = branchLength;
            return Underflow{leftSiblingReal, UnderflowResolution::REDISTRIBUTE};
        }
        else if (rightSibling != nullptr)
        {
            // move the tail pointer of self over to make room
            m_Pointers[branchLength + 1] = m_Pointers[1];

            // copy right entry's first K/2 entries over
            memcpy(m_Keys + 1, rightSiblingReal->m_Keys, branchLength * sizeof(uint64_t));
            memcpy(m_Pointers + 1, rightSiblingReal->m_Pointers, branchLength * sizeof(void *));
            m_Fill = branchLength + 1;

            // move the last branchLength items in rightSibling forward
            memcpy(rightSiblingReal->m_Keys, rightSiblingReal->m_Keys + branchLength, branchLength * sizeof(uint64_t));
            memcpy(rightSiblingReal->m_Pointers, rightSiblingReal->m_Pointers + branchLength,
                   branchLength * sizeof(void *));
            rightSiblingReal->m_Fill = branchLength;

            return Underflow{rightSiblingReal, UnderflowResolution::REDISTRIBUTE};
        }
        else
        {
            return Underflow{nullptr, UnderflowResolution::UNRESOLVED_LEAF};
        }
    }
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

bool Engine::Core::Memory::BpTree::Remove(uint64_t key, void *&outValue)
{
    Underflow underflow = m_Root->TryRemove(key, outValue, nullptr, nullptr);
    switch (underflow.Resolution)
    {
    case UnderflowResolution::NO_DELETION:
        return false;
    case UnderflowResolution::NO_UNDERFLOW:
        return true;
    case UnderflowResolution::UNRESOLVED_LEAF:
        return true;
    case UnderflowResolution::UNRESOLVED_INTERNAL: {
        InternalNode *rootReal = static_cast<InternalNode *>(m_Root);
        m_Root = static_cast<IBpNode *>(rootReal->m_Pointers[0]);
        return true;
    }
    default:
        // TODO: what to do with an unexpected result?
        SE_THROW_NOT_IMPLEMENTED;
    }
}

bool Engine::Core::Memory::BpTree::TryGet(uint64_t key, void *&outValue) const
{
    if (m_Root == nullptr)
        return false;

    return m_Root->TryGet(key, outValue);
}

void Engine::Core::Memory::LeafNode::Print(std::stringstream &stream) const
{
    stream << "leaf <" << std::hex << (unsigned long long)this << '>' << std::endl;

    for (int i = 0; i < m_Fill; i++)
    {
        stream << m_Keys[i] << ":" << std::hex << (unsigned long long)m_Pointers[i] << " ";
    }

    stream << "->[" << std::hex << (unsigned long long)m_Pointers[m_Fill] << ']' << std::endl << std::endl;
}

void Engine::Core::Memory::InternalNode::Print(std::stringstream &stream) const
{
    stream << "internal <" << std::hex << (unsigned long long)this << ">" << std::endl;
    for (int i = 0; i < m_Fill; i++)
    {
        stream << '[' << std::hex << (unsigned long long)m_Pointers[i] << "] " << m_Keys[i] << " ";
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
