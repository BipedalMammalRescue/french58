#include "bp_tree.h"
#include "Configuration/compile_time_flags.h"
#include "ErrorHandling/exceptions.h"
#include "high_integrity_allocator.h"
#include <cstring>

using namespace Engine::Core;
using namespace Engine::Core::Memory;

namespace Engine {
namespace Core {
namespace Memory {
    
struct BpLink
{
    void* Left = nullptr;
    unsigned long long Key = 0;
};


struct BpNode
{
    BpNode* Parent = nullptr;

    BpLink Links[Configuration::B_PLUS_TREE_K_ARY - 1];
    BpNode* Right = nullptr;

    // there's always one more valid pointer than valid keys
    unsigned int ValidKeyCount;
    
    unsigned int FindSlot(unsigned long long candidate) const;

    inline bool IsFull() const 
    {
        return ValidKeyCount == Configuration::B_PLUS_TREE_K_ARY - 1;
    }
};

}}}


unsigned int BpNode::FindSlot(unsigned long long candidate) const
{
    for (int i = 0; i < ValidKeyCount; i++) 
    {
        if (candidate <= Links[i].Key)
            return i;
    }

    return ValidKeyCount;
}


BpNode* BpTree::LeafLookup(unsigned long long key)
{
    BpNode* current = m_Head;
    unsigned int depth = 0;
    while (depth < m_Depth) 
    {
        unsigned int slot = current->FindSlot(key);
        BpNode* next = nullptr;
        if (slot < current->ValidKeyCount)
        {
            if (current->Links[slot].Left == nullptr)
            {
                current->Links[slot].Left = m_Allocator->New<BpNode>();
            }

            next = (BpNode*) current->Links[slot].Left;
        }
        else 
        {
            if (current->Right == nullptr)
            {
                current->Right = m_Allocator->New<BpNode>();
            }

            next = (BpNode*) current->Right;
        }

        current = next;
    }

    return current;
}


struct SplitNodeResults
{
    BpLink* Overflow;
    BpNode* Left;
    BpNode* Right;
};


static SplitNodeResults SplitNode(BpNode* targetNode, BpLink* newLink, HighIntegrityAllocator* allocator, bool isLeaf)
{
    // resolve overflow
    // calculate the "split"
    unsigned int branchLength = Configuration::B_PLUS_TREE_K_ARY / 2;
    BpLink* overflownSlots[Configuration::B_PLUS_TREE_K_ARY] {0};
    memset(overflownSlots, 0, Configuration::B_PLUS_TREE_K_ARY);

    // note: we are not actually moving the pointers around but copying the data, so it's ok to create a link on stack and pass its address around
    // copy the inserted version of every link into a stack buffer
    unsigned int slot = targetNode->FindSlot(newLink->Key);
    overflownSlots[slot] = newLink;

    unsigned int cursor = 0;
    for (int i = 0; i < Configuration::B_PLUS_TREE_K_ARY - 1; i++) 
    {
        if (overflownSlots[i] != nullptr)
            continue;

        overflownSlots[i] = &targetNode->Links[cursor];
        cursor++;
    }

    // allocate new leaf node
    BpNode* newNode = allocator->New<BpNode>();
    newNode->Right = targetNode->Right;
    targetNode->Right = isLeaf ? newNode : nullptr;

    // copy the tail from overflown buffer into the new node
    for (int i = branchLength + 1; i < Configuration::B_PLUS_TREE_K_ARY; i++)
    {
        newNode->Links[i - branchLength - 1] = *(overflownSlots[i]);
    }

    // clear the tail from target leaf
    targetNode->ValidKeyCount = branchLength + 1;
    newNode->ValidKeyCount = branchLength;

    // in case the new node is in the middle, re-assign the last valid link in the target leaf
    // does it count as optimization since I technically avoided copying the previous entries?
    targetNode->Links[branchLength] = *overflownSlots[branchLength];

    // it should be ok to pass pointer here since it points to a part of memory that's already done writing
    return SplitNodeResults { &targetNode->Links[branchLength], targetNode, newNode };
}


void BpTree::ResolveInteralOverflow(BpLink* overflowLink, BpNode* targetParent, BpNode* leftChild, BpNode* rightChild)
{
    if (targetParent != nullptr && !targetParent->IsFull()) 
    {
        // easy case, slot the new link in and call it
        unsigned int slot = targetParent->FindSlot(overflowLink->Key);
        for (int i = targetParent->ValidKeyCount - 1; i >= slot; i--)
        {
            targetParent->Links[i + 1] = targetParent->Links[i];
        }
        targetParent->Links[slot] = *overflowLink;
        targetParent->ValidKeyCount++;

        // assign the new two children into the new slot
        targetParent->Links[slot].Left = leftChild;
        if (slot == targetParent->ValidKeyCount - 1)
        {
            targetParent->Right = rightChild;
        }
        else 
        {
            targetParent->Links[slot + 1].Left = rightChild;
        }
    }
    else if (targetParent != nullptr)
    {
        // parent overflow
        SplitNodeResults splitResults = SplitNode(targetParent, overflowLink, m_Allocator, false);
        splitResults.Left->Links[splitResults.Left->ValidKeyCount - 1].Left = leftChild;
        splitResults.Left->Links[splitResults.Left->ValidKeyCount].Left = rightChild;

        ResolveInteralOverflow(splitResults.Overflow, targetParent->Parent, splitResults.Left, splitResults.Right);
    }
    else
    {

        // TODO: create new root
        SE_THROW_NOT_IMPLEMENTED;
    }
}


bool BpTree::Insert(unsigned long long key, void* value)
{
    if (m_Head == nullptr)
    {
        // initialize a new node
        m_Head = m_Allocator->New<BpNode>();
        m_Head->Links[0].Key = key;
        m_Head->Links[0].Left = value;
        m_Head->ValidKeyCount = 1;
        return true;
    }

    BpNode* targetLeaf = LeafLookup(key);
    unsigned int slot = targetLeaf->FindSlot(key);
    if (targetLeaf->Links[slot].Key == key)
        return false;

    // insert the new link
    if (!targetLeaf->IsFull()) 
    {
        // no overflow
        for (int i = targetLeaf->ValidKeyCount - 1; i >= slot; i--)
        {
            targetLeaf->Links[i + 1] = targetLeaf->Links[i];
        }

        targetLeaf->Links[slot].Key = key;
        targetLeaf->Links[slot].Left = value;
    }
    else 
    {
        // split the leaf node into two and record the results
        BpLink insertedLink {value, key};
        SplitNodeResults splitResult = SplitNode(targetLeaf, &insertedLink, m_Allocator, true);

        // start sending overflow upwards
        ResolveInteralOverflow(splitResult.Overflow, targetLeaf->Parent, splitResult.Left, splitResult.Right);
    }

    return false;
}
