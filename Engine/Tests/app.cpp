#include "Memory/bp_tree.h"
#include "Memory/cache_friendly_allocator.h"
#include "Memory/high_integrity_allocator.h"
#include <exception>
#include <iostream>

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

bool BudgetAllocatorTest()
{
    struct Data
    {
        int a = 0;
        int b = 0;
    };

    Engine::Core::Memory::CacheFriendlyAllocator<Data, 64> allocator;
    std::cout << sizeof(Engine::Core::Memory::CacheFriendlyAllocator<Data, 64>) << std::endl;

    Data *value1 = allocator.Malloc();
    Data *value2 = allocator.Malloc();
    Data *value3 = allocator.Malloc();
    value1->a = 100;
    value2->a = 100;
    value3->a = 100;

    int total = 0;

    allocator.IterateAll([&total](const Data *cursor) { total += cursor->a; });

    assert(total == 300);

    return true;
}

int main()
{
    // SE_TEST_RUNTEST(BpTreeTest);
    SE_TEST_RUNTEST(BudgetAllocatorTest);
    std::cout << "done" << std::endl;
    return 0;
}
