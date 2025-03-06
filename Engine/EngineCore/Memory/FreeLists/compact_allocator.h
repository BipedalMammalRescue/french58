

namespace Engine {
namespace Core {
namespace Memory {
namespace FreeLists {

// Compact allocator allocates everything in comapct (no padding between allocated elements).
// Due to the simpler logic, the only size limitation is 8bytes lower bound (again, sub-word allocation should not use
// free list allocator)
template <typename T, size_t TItemsPerBlock = 64, size_t TInitialBlocks = 1, size_t TBlocksPerAllocation = 1>
class CompactAllocator
{
  private:
    union Item {
        T Data;
        Item *NextFree;
    };

    struct Block
    {
        Item Items[TItemsPerBlock];
    };
};

} // namespace FreeLists
} // namespace Memory
} // namespace Core
} // namespace Engine
