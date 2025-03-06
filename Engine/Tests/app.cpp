#include "Memory/FreeLists/bucket_allocator.h"
#include "Memory/bp_tree.h"
#include "Memory/high_integrity_allocator.h"
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

bool BpTreeTest()
{
    Engine::Core::Memory::HighIntegrityAllocator allocator(16);

    {
        Engine::Core::Memory::BpTree tree(&allocator);

        constexpr int ELEMENT_COUNT = 5000;

        int data[ELEMENT_COUNT];

        for (int i = 0; i < ELEMENT_COUNT; i++)
        {
            data[i] = i * 155;
            tree.Insert(i, &data[i]);
        }

        // print out the tree
        std::string outString;
        tree.Print(outString);
        std::cout << outString << std::endl;

        // test the values
        for (int i = 0; i < ELEMENT_COUNT; i++)
        {
            void *ptr;
            assert(tree.TryGet(i, ptr));
            assert(*((int *)ptr) == i * 155);
        }

        // test deletion

        return true;
    }
}

bool BucketAllocatorTest()
{
    struct Data
    {
        int a = 0;
        int b = 0;
    };

    std::cout << "sizeof(BucketAllocator) = " << sizeof(Engine::Core::Memory::FreeLists::BucketAllocator<Data>)
              << std::endl;

    Engine::Core::Memory::FreeLists::BucketAllocator<Data> allocator;

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

int main()
{
    // SE_TEST_RUNTEST(BpTreeTest);
    // SE_TEST_RUNTEST(CacheLineAllocatorTest);
    SE_TEST_RUNTEST(BucketAllocatorTest);
    std::cout << "DONE" << std::endl;
    return 0;
}
