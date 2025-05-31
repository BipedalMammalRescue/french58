#pragma once

#include "EngineUtils/Memory/programmable_configs.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>

namespace Engine::Utils::Memory::FreeList {

template <typename T, size_t TBucketsPerBlock = 64, typename TGrowthFactor = QuadraticGrowth<2>> class BucketAllocator
{
  private:
    static constexpr size_t GetBucketSize()
    {
        // some static checks
        static_assert(sizeof(T) >= 8 && sizeof(T) <= 64,
                      "BucketAllocator client type size out of range (8~64 bytes)! (Note: sub-word objects should "
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

    size_t m_FreeListHead = 1;
    Block **m_Blocks = nullptr;
    size_t m_BlockCapacity = 0;

    void EnsureAllocationAvailable()
    {
        size_t nextAllocation = TGrowthFactor::NextAllocationSize(m_BlockCapacity);
        if (nextAllocation == 0 || m_FreeListHead < m_BlockCapacity * TBucketsPerBlock)
            return;

        // extend the block table
        m_Blocks = (Block **)realloc(m_Blocks, sizeof(Block *) * (m_BlockCapacity + nextAllocation));
        assert(m_Blocks != nullptr);

        // register the new blocks
        Block *newBlocks = (Block *)malloc(sizeof(Block) * nextAllocation);
        assert(newBlocks != nullptr);
        for (size_t i = 0; i < nextAllocation; i++)
        {
            m_Blocks[i + m_BlockCapacity] = newBlocks + i;
        }

        // initialize new blocks
        for (size_t i = m_BlockCapacity; i < m_BlockCapacity + nextAllocation - 1; i++)
        {
            newBlocks[i - m_BlockCapacity].Reset(i * TBucketsPerBlock, (i + 1) * TBucketsPerBlock);
        }
        m_Blocks[m_BlockCapacity + nextAllocation - 1]->Reset((m_BlockCapacity + nextAllocation - 1) * TBucketsPerBlock,
                                                              (m_BlockCapacity + nextAllocation) * TBucketsPerBlock);

        // seal the deal by incrementing block capacity and free list
        m_FreeListHead = m_BlockCapacity * TBucketsPerBlock;
        m_BlockCapacity += nextAllocation;
    }

    template <typename TFunc> static void IterateBlockCluster(size_t clusterSize, Block *blocks, TFunc &&lambda)
    {
        for (size_t i = 0; i < clusterSize; i++)
        {
            blocks[i].Iterate(lambda);
        }
    }

  public:
    BucketAllocator()
    {
        EnsureAllocationAvailable();
    }

    ~BucketAllocator()
    {
        size_t currentAllocationSize = 0;
        for (size_t i = 0; i < m_BlockCapacity; i += currentAllocationSize)
        {
            currentAllocationSize = TGrowthFactor::NextAllocationSize(i);
            free(m_Blocks[i]);
        }
        free(m_Blocks);
    }

    size_t New(const T &value)
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
        size_t clusterSize;

        for (size_t i = 0; i < m_BlockCapacity; i += clusterSize)
        {
            clusterSize = TGrowthFactor::NextAllocationSize(i);
            BucketAllocator::IterateBlockCluster(clusterSize, m_Blocks[i], lambda);
        }
    }
};

}