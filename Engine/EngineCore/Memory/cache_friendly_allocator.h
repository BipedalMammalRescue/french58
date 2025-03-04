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

    inline BudgetBlock *&GetHead()
    {
        return m_Blocks[0];
    }

    inline BudgetBlock *&GetTail()
    {
        return m_Blocks[m_BlockCount - 1];
    }

    BudgetBlock **m_Blocks =
        (BudgetBlock **)malloc(Configuration::ALLOCATOR_BLOCK_LIST_INITIAL_CAPACITY * sizeof(void *));
    Configuration::AllocatorIndexType m_BlockCapacity = Configuration::ALLOCATOR_BLOCK_LIST_INITIAL_CAPACITY;
    Configuration::AllocatorIndexType m_BlockCount = 1;
    Configuration::AllocatorIndexType m_FreeHead = 0;

  public:
    CacheFriendlyAllocator()
    {
        static_assert(sizeof(BudgetBlock::BudgetBlockHeader) != sizeof(CacheUnit),
                      "Number of items per block exceeds cache line size limit!");

        GetHead() = (BudgetBlock *)malloc(sizeof(BudgetBlock));

        GetHead()->Header.Data.NextBlock = nullptr;
        GetHead()->Header.Data.Bitmask = 0;

        // initialize free block
        GetHead()->Reset(0);
    }

    ~CacheFriendlyAllocator()
    {
        BudgetBlock *nextBlock = nullptr;
        BudgetBlock *currentBlock = GetHead();
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
        BudgetBlock *currentBlock = GetHead();
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
        BudgetBlock *targetBlock = GetHead();
        BudgetBlock *nextBlock = nullptr;

        if (m_FreeHead == Configuration::INVALID_ALLOCATOR_INDEX)
        {
            // grow the block list if it's needed
            if (m_BlockCount == m_BlockCapacity)
            {
                realloc(m_Blocks, m_BlockCapacity * 2);
                m_BlockCapacity *= 2;
            }

            // add a new block
            BudgetBlock *newBlock = (BudgetBlock *)malloc(sizeof(BudgetBlock));
            GetTail()->Header.Data.NextBlock = newBlock;

            // append the new block
            m_BlockCount++;
            GetTail() = newBlock;

            // initialize new block
            newBlock->ResetAndPrepend((m_BlockCount - 1) * TItemsPerBlock, Configuration::INVALID_ALLOCATOR_INDEX);
            BudgetBlock::SetValidity(newBlock->Header.Data.Bitmask, m_BlockCount * TItemsPerBlock);

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

    void Free(Configuration::AllocatorIndexType item)
    {
    }

    template <typename TFunc> void IterateAll(TFunc &&function)
    {
        BudgetBlock *nextBlock = nullptr;
        BudgetBlock *currentBlock = GetHead();
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
        BudgetBlock *currentBlock = GetHead();
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
