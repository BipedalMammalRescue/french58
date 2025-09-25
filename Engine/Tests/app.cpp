#define DEBUG_OR_TEST 0b10

#include <EngineUtils/Memory/FreeList/bucket_allocator.h>
#include <EngineUtils/Memory/FreeList/compact_allocator.h>
#include <EngineUtils/Memory/Lifo/unmanaged_stack_allocator.h>
#include <EngineUtils/Memory/alignment_calc.h>
#include <cassert>
#include <exception>
#include <iostream>
#include <vector>

#define SE_TEST_RUNTEST(testName)                                                                                      \
    try                                                                                                                \
    {                                                                                                                  \
        bool result = testName();                                                                                      \
        std::cout << "-> ";                                                                                            \
        if (!result)                                                                                                   \
        {                                                                                                              \
            std::cout << #testName;                                                                                    \
            SetColor(31);                                                                                              \
            std::cout << " FAIL" << std::endl;                                                                         \
            ResetColor();                                                                                              \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            std::cout << #testName;                                                                                    \
            SetColor(32);                                                                                              \
            std::cout << " SUCCESS";                                                                                   \
            ResetColor();                                                                                              \
            std::cout << std::endl;                                                                                    \
        }                                                                                                              \
    }                                                                                                                  \
    catch (const std::exception &ex)                                                                                   \
    {                                                                                                                  \
        SetColor(31);                                                                                                  \
        std::cout << "fail: " << #testName << " [err: " << ex.what() << "]" << std::endl;                              \
        ResetColor();                                                                                                  \
    }

void SetColor(int textColor)
{
    std::cout << "\033[" << textColor << "m";
}

void ResetColor()
{
    std::cout << "\033[0m";
}

bool BucketAllocatorTest()
{
    struct Data
    {
        int a = 0;
        int b = 0;
    };

    std::cout << "sizeof(BucketAllocator) = " << sizeof(Engine::Utils::Memory::FreeList::BucketAllocator<Data>)
              << std::endl;

    Engine::Utils::Memory::FreeList::BucketAllocator<Data> allocator;

    const size_t TRIALS = 1000000;
    std::vector<size_t> ids;
    ids.reserve(TRIALS);

    for (size_t i = 0; i < TRIALS; i++)
    {
        ids.push_back(allocator.New({1, 0}));
    }

    int total = 0;

    allocator.IterateAll([&total](const Data *cursor) { total += cursor->a; });

    assert(total == TRIALS);

    for (size_t id : ids)
    {
        if (id % 2 == 0)
            continue;

        allocator.Free(id);
    }

    total = 0;
    allocator.IterateAll([&total](const Data *cursor) { total += cursor->a; });
    assert(total == TRIALS / 2);

    return true;
}

bool StackAllocatorTest()
{
    using namespace Engine::Utils::Memory::Lifo;

    UnmanagedStackAllocator allocator(16);

    size_t offsetOne = allocator.Allocate(16);

    unsigned char *bufferOne = allocator.GetPointer(offsetOne);
    for (unsigned char i = 0; i < 16; i++)
    {
        bufferOne[i] = i;
    }

    assert(allocator.DebugGetCapacity() == 16);

    size_t offsetTwo = allocator.Allocate(5);
    assert(allocator.DebugGetCapacity() == 32);

    unsigned char *bufferTwo = allocator.GetPointer(offsetTwo);
    for (unsigned char i = 0; i < 5; i++)
    {
        bufferTwo[i] = i + 16;
    }

    unsigned char *bufferOneNew = allocator.GetPointer(offsetOne);
    for (unsigned char i = 0; i < 16; i++)
    {
        assert(bufferOneNew[i] == i);
    }

    allocator.Deallocate(offsetOne);
    assert(allocator.DebugGetOffset() == 0);

    return true;
}

bool MemoryAlignmentTest() {
    using namespace Engine::Utils::Memory;

    for (size_t i = 1; i < 0xFFFFFFFF; i++) 
    {
        size_t expected = i * 8;

        for (size_t j = 1; j < 8; j++) 
        {
            if (AlignLength((i - 1) * 8 + j, 8) != expected)
                return false;
        }
    }

    return true;
}

int main()
{
    // SE_TEST_RUNTEST(BpTreeTest);
    // SE_TEST_RUNTEST(CacheLineAllocatorTest);
    // SE_TEST_RUNTEST(BucketAllocatorTest);

    // SE_TEST_RUNTEST(StackAllocatorTest);

    SE_TEST_RUNTEST(MemoryAlignmentTest);

    std::cout << "DONE" << std::endl;
    return 0;
}
