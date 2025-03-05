#include <cassert>
#include <cstdint>
#include <cstdlib>

namespace Engine {
namespace Core {
namespace Memory {

template <typename T, size_t TBucketsPerBlock = 64, size_t TBlocksPerAllocation = 1, size_t TInitialBlocks = 1>
class BucketAllocator
{
  private:
    static constexpr size_t GetBucketSize()
    {
        // some static checks
        static_assert(sizeof(T) >= 8 && sizeof(T) <= 64,
                      "BucketAllocator client type size out of range (8~128 bytes)! (Note: sub-word objects should "
                      "probably use a circular buffer)");

        if (sizeof(T) <= 8)
            return 8;
        else if (sizeof(T) <= 16)
            return 16;
        else if (sizeof(T) <= 32)
            return 32;
        else if (sizeof(T) <= 64)
            return 64;
    }

    union HeadBucket {
        char __alignment[GetBucketSize()];
        uint64_t Bitmask;
    };

    union Bucket {
        char __alignment[GetBucketSize()];
        size_t NextFree;
        T Data;
    };

    struct Block
    {
        HeadBucket Head;
        Bucket Buckets[TBucketsPerBlock];

        void Reset(size_t offset, size_t freeListHead)
        {
            Head.Bitmask = 0;

            for (int i = 0; i < TBucketsPerBlock - 1; i++)
            {
                Buckets[i].NextFree = offset + i + 1;
            }

            Buckets[TBucketsPerBlock - 1].NextFree = freeListHead;
        }

        template <typename TFunc> void Iterate(TFunc &&lambda)
        {
            uint64_t mask = Head.Bitmask;
            if (mask == 0)
                return;

            for (size_t i = 0; i < TBucketsPerBlock; i++)
            {
                if ((mask & (static_cast<uint64_t>(1) << i)) == 0)
                    continue;

                lambda(&Buckets[i].Data);
            }
        }
    };

    size_t m_FreeListHead = -1;
    Block **m_Blocks = nullptr;
    size_t m_BlockCapacity = 0;

    Block *AllocateNewBatch()
    {
        return malloc(sizeof(Block) * TBlocksPerAllocation);
    }

    void EnsureAllocationAvailable()
    {
        if (TBlocksPerAllocation == 0 || m_FreeListHead >= 0)
            return;

        // extend the block table
        m_Blocks = (Block **)realloc(m_Blocks, sizeof(Block *) * (m_BlockCapacity + TBlocksPerAllocation));

        // register the new blocks
        Block *newBlocks = (Block *)malloc(sizeof(Block) * TBlocksPerAllocation);
        for (int i = m_BlockCapacity; i < m_BlockCapacity + TBlocksPerAllocation; i++)
        {
            m_Blocks[i] = newBlocks + i;
        }

        // initialize new blocks
        for (size_t i = m_BlockCapacity; i < m_BlockCapacity + TBlocksPerAllocation - 1; i++)
        {
            newBlocks[i - m_BlockCapacity].Reset(i * TBucketsPerBlock, (i + 1) * TBucketsPerBlock);
        }
        m_Blocks[m_BlockCapacity + TBlocksPerAllocation - 1]->Reset(
            (m_BlockCapacity + TBlocksPerAllocation - 1) * TBucketsPerBlock, -1);

        // seal the deal by incrementing block capacity and free list
        m_FreeListHead = m_BlockCapacity * TBucketsPerBlock;
        m_BlockCapacity += TBlocksPerAllocation;
    }

  public:
    BucketAllocator()
    {
        // allocate block table
        m_Blocks = (Block **)malloc(sizeof(Block *) * TInitialBlocks);
        m_BlockCapacity = TInitialBlocks;

        // allocate starting blocks
        Block *initialBlocks = (Block *)malloc(sizeof(Block) * TInitialBlocks);
        for (size_t i = 0; i < m_BlockCapacity; i++)
        {
            m_Blocks[i] = initialBlocks + i;
        }

        // initialize blocks
        for (size_t i = 0; i < m_BlockCapacity - 1; i++)
        {
            m_Blocks[i]->Reset(i * TBucketsPerBlock, (i + 1) * TBucketsPerBlock);
        }

        // initialize the last block
        m_Blocks[m_BlockCapacity - 1]->Reset((m_BlockCapacity - 1) * TBucketsPerBlock, -1);

        // initialize free list
        m_FreeListHead = 0;
    }

    ~BucketAllocator()
    {
        free(m_Blocks[0]);
        for (size_t i = TInitialBlocks; i < m_BlockCapacity; i += TBlocksPerAllocation)
        {
            free(m_Blocks[i]);
        }
        free(m_Blocks);
    }

    size_t New(const T &value)
    {
        EnsureAllocationAvailable();

        // soft throw if allocator can't grow further (e.g. user turns off extra allocation past initial blocks)
        if (m_FreeListHead < 0)
            return -1;

        // take, mark bitmask, and update the free list
        size_t targetIndex = m_FreeListHead;
        m_Blocks[targetIndex / TBucketsPerBlock]->Head.Bitmask |= (uint64_t)1 << targetIndex % TBucketsPerBlock;
        Bucket *allocationTarget = m_Blocks[targetIndex / TBucketsPerBlock]->Buckets[targetIndex % TBucketsPerBlock];
        m_FreeListHead = allocationTarget->NextFree;

        allocationTarget->Data = value;

        return targetIndex;
    }

    size_t New(const T &&value)
    {
        EnsureAllocationAvailable();

        // take, mark bitmask, and update the free list
        size_t targetIndex = m_FreeListHead;
        m_Blocks[targetIndex / TBucketsPerBlock]->Head.Bitmask |= (uint64_t)1 << targetIndex % TBucketsPerBlock;
        Bucket *allocationTarget = &m_Blocks[targetIndex / TBucketsPerBlock]->Buckets[targetIndex % TBucketsPerBlock];
        m_FreeListHead = allocationTarget->NextFree;

        allocationTarget->Data = value;

        return targetIndex;
    }

    void Free(size_t index)
    {
        // clear bitmask
        Block *targetBlock = m_Blocks[index / TBucketsPerBlock];
        targetBlock->Head.Bitmask &= ~((uint64_t)1 << index % TBucketsPerBlock);

        // find the bucket
        Bucket *freeTarget = &targetBlock->Buckets[index % TBucketsPerBlock];

        // update the free list
        freeTarget->NextFree = m_FreeListHead;
        m_FreeListHead = index;
    }

    template <typename TFunc> void IterateAll(TFunc &&lambda)
    {
        // take advantage of the batch allocations
        // iterate through the initial batch
        Block *currentBlock = m_Blocks[0];
        uint64_t currentMask = 0;
        for (size_t i = 0; i < TInitialBlocks; i++)
        {
            Block *targetBlock = currentBlock + i;
            targetBlock->Iterate(lambda);
        }

        // iterate through the later batches
        for (size_t i = TInitialBlocks; i < m_BlockCapacity; i += TBlocksPerAllocation)
        {
            currentBlock = m_Blocks[i];

            for (size_t j = 0; j < TBlocksPerAllocation; j++)
            {
                Block *targetBlock = currentBlock + j;
                targetBlock->Iterate(lambda);
            }
        }
    }
};

} // namespace Memory
} // namespace Core
} // namespace Engine
