#include "Configuration/compile_time_flags.h"
#include <cassert>
#include <cstdlib>

namespace Engine {
namespace Core {
namespace Memory {

// a struct the size of a cache line
struct CacheUnit
{
    unsigned char Buffer[Configuration::ALLOCATOR_CACHELINE_SIZE];
};

template <typename TItem, size_t TItemsPerBlock> class CacheFriendlyAllocator
{
  private:
    static constexpr size_t GetItemsPerUnit()
    {
        static_assert(sizeof(TItem) <= Configuration::ALLOCATOR_CACHELINE_SIZE,
                      "Allocating items of size greater than cache line is not allowed in BudgetAllocator!");
        return Configuration::ALLOCATOR_CACHELINE_SIZE / sizeof(TItem);
    }

    static constexpr size_t GetUnitsPerBlock()
    {
        static_assert(TItemsPerBlock <= Configuration::ALLOCATOR_BITMASK_WIDTH,
                      "Allocating more than bitmask width items per block is not allowed in BudgetAllocator!");
        return (TItemsPerBlock + GetItemsPerUnit() - 1) / GetItemsPerUnit();
    }

    union FreeListItem {
        Configuration::AllocatorIndexType NextFree;
        TItem Item;
    };

    // an array of CacheUnits serving as a unit of allocation,
    // and a header that's aligned into a cache line unit too
    struct BudgetBlock
    {
        struct BudgetBlockHeader
        {
            BudgetBlock *NextBlock = nullptr;
            Configuration::AllocatorBitmaskType Bitmask;
        };

        union {
            CacheUnit __alignment;
            BudgetBlockHeader Data;
        } Header;

        union {
            CacheUnit __alignment;
            FreeListItem Data[GetItemsPerUnit()];
        } Units[GetUnitsPerBlock()];

        void Reset(Configuration::AllocatorIndexType offset)
        {
            Header.Data.Bitmask = 0;
            bool isLast = Header.Data.NextBlock == nullptr;

            for (int i = 0; i < TItemsPerBlock - 1; i++)
            {
                Units[i / GetItemsPerUnit()].Data[i % GetItemsPerUnit()].NextFree = offset + i + 1;
            }

            Units[GetUnitsPerBlock() - 1].Data[GetItemsPerUnit() - 1].NextFree =
                isLast ? Configuration::INVALID_ALLOCATOR_INDEX : offset + TItemsPerBlock;
        }

        void ResetAndPrepend(Configuration::AllocatorIndexType offset,
                             Configuration::AllocatorIndexType previousFreeHead)
        {
            Header.Data.Bitmask = 0;
            bool isLast = Header.Data.NextBlock == nullptr;

            for (int i = 0; i < TItemsPerBlock - 1; i++)
            {
                *(Configuration::AllocatorBitmaskType *)(&Units[i / GetItemsPerUnit()].Data[i % GetItemsPerUnit()]) =
                    offset + i + 1;
            }

            *(Configuration::AllocatorBitmaskType *)(&Units[GetUnitsPerBlock() - 1].Data[GetItemsPerUnit() - 1]) =
                previousFreeHead;
        }

        static void SetValidity(Configuration::AllocatorBitmaskType &bitmask, Configuration::AllocatorIndexType offset)
        {
            bitmask |= (static_cast<Configuration::AllocatorBitmaskType>(2) << offset);
        }

        static void ClearValidity(Configuration::AllocatorBitmaskType &bitmask,
                                  Configuration::AllocatorIndexType offset)
        {
            bitmask &= ~(static_cast<Configuration::AllocatorBitmaskType>(2) << offset);
        }

        // map the index into a location in the bitmap, and test if that location is populated
        static Configuration::AllocatorBitmaskType CheckValidity(Configuration::AllocatorBitmaskType bitmask,
                                                                 Configuration::AllocatorIndexType offset)
        {
            return bitmask & (static_cast<Configuration::AllocatorBitmaskType>(2) << offset);
        }
    };

    BudgetBlock *m_Head = (BudgetBlock *)malloc(sizeof(BudgetBlock));
    BudgetBlock *m_Tail = m_Head;
    Configuration::AllocatorIndexType m_BlockCount = 1;
    Configuration::AllocatorIndexType m_FreeHead = 0;

  public:
    CacheFriendlyAllocator()
    {
        static_assert(sizeof(BudgetBlock::BudgetBlockHeader) != sizeof(CacheUnit),
                      "Number of items per block exceeds cache line size limit!");

        m_Head[0].Header.Data.NextBlock = nullptr;
        m_Head[0].Header.Data.Bitmask = 0;

        // initialize free block
        m_Head[0].Reset(0);
    }

    ~CacheFriendlyAllocator()
    {
        BudgetBlock *nextBlock = nullptr;
        BudgetBlock *currentBlock = m_Head;
        Configuration::AllocatorBitmaskType currentBitmask = 0;

        while (currentBlock != nullptr)
        {
            nextBlock = currentBlock->Header.Data.NextBlock;
            free(currentBlock);
            currentBlock = nextBlock;
        }
    }

    void Reset()
    {
        BudgetBlock *nextBlock = nullptr;
        BudgetBlock *currentBlock = m_Head;
        Configuration::AllocatorIndexType offset = 0;
        while (currentBlock != nullptr)
        {
            nextBlock = currentBlock->Header.Data.NextBlock;
            currentBlock->Reset(offset);
            currentBlock = nextBlock;
            offset += TItemsPerBlock;
        }
    }

    TItem *Malloc()
    {
        BudgetBlock *targetBlock = m_Head;
        BudgetBlock *nextBlock = nullptr;

        if (m_FreeHead == Configuration::INVALID_ALLOCATOR_INDEX)
        {
            // add a new block
            BudgetBlock *newBlock = (BudgetBlock *)malloc(sizeof(BudgetBlock));
            m_Tail->Header.Data.NextBlock = newBlock;
            newBlock->ResetAndPrepend(m_BlockCount * TItemsPerBlock, Configuration::INVALID_ALLOCATOR_INDEX);
            BudgetBlock::SetValidity(newBlock->Header.Data.Bitmask, m_BlockCount * TItemsPerBlock);
            m_Tail = newBlock;
            m_BlockCount++;

            // get the new item from newBlock directly
            FreeListItem *result = &newBlock->Units[0].Data[0];
            m_FreeHead = result->NextFree;
            return &result->Item;
        }

        Configuration::AllocatorIndexType allocateeId = m_FreeHead;
        Configuration::AllocatorIndexType blockId = allocateeId / TItemsPerBlock;
        Configuration::AllocatorIndexType blockOffset = allocateeId % TItemsPerBlock;

        for (Configuration::AllocatorIndexType i = 0; i < blockId; i++)
        {
            nextBlock = targetBlock->Header.Data.NextBlock;
            targetBlock = nextBlock;
        }

        // past this line only access the allocatee
        BudgetBlock::SetValidity(targetBlock->Header.Data.Bitmask, blockOffset);
        FreeListItem *targetItem =
            &targetBlock->Units[blockOffset / GetItemsPerUnit()].Data[blockOffset % GetItemsPerUnit()];
        m_FreeHead = targetItem->NextFree;
        return &targetItem->Item;
    }

    void Free(TItem *item)
    {
    }

    template <typename TFunc> void IterateAll(TFunc &&function)
    {
        BudgetBlock *nextBlock = nullptr;
        BudgetBlock *currentBlock = m_Head;
        Configuration::AllocatorBitmaskType currentBitmask = 0;

        // for each block
        while (currentBlock != nullptr)
        {
            nextBlock = currentBlock->Header.Data.NextBlock;
            currentBitmask = currentBlock->Header.Data.Bitmask;

            // skip the block if the bitmask doesn't have values
            if (currentBlock->Header.Data.Bitmask == 0)
            {
                currentBlock = nextBlock;
                continue;
            }

            // for each possible item index
            for (Configuration::AllocatorBitmaskType i = 0; i < Configuration::ALLOCATOR_BITMASK_WIDTH; i++)
            {
                if (BudgetBlock::CheckValidity(currentBitmask, i) == 0)
                    continue;

                // TODO: make sure everything below this line only deals with the struct at this location
                TItem *cursor = &currentBlock->Units[i / GetItemsPerUnit()].Data[i % GetItemsPerUnit()].Item;
                function(cursor);
            }

            currentBlock = nextBlock;
        }
    }

    template <typename TFunc> void IterateAll(TFunc &&function) const
    {
        BudgetBlock *nextBlock = nullptr;
        BudgetBlock *currentBlock = m_Head;
        Configuration::AllocatorBitmaskType currentBitmask = 0;

        // for each block
        while (currentBlock != nullptr)
        {
            nextBlock = currentBlock->Header.Data.NextBlock;
            currentBitmask = currentBlock->Header.Data.Bitmask;

            // skip the block if the bitmask doesn't have values
            if (currentBlock->Header.Data.BitMask == 0)
            {
                currentBlock = nextBlock;
                continue;
            }

            // for each possible item index
            for (Configuration::AllocatorBitmaskType i = 0; i < Configuration::ALLOCATOR_BITMASK_WIDTH; i++)
            {
                if (BudgetBlock::CheckValidity(currentBitmask, i) == 0)
                    continue;

                // TODO: make sure everything below this line only deals with the struct at this location
                const TItem *cursor =
                    static_cast<TItem *>(currentBlock->Units[i / GetItemsPerUnit()]) + (i % GetItemsPerUnit());
                function(cursor);
            }
        }
    }
};

} // namespace Memory
} // namespace Core
} // namespace Engine
