#pragma once

#include <cstddef>
#include <cstdint>
namespace Engine {
namespace Core {
namespace Configuration {

constexpr long long STRING_LOAD_BUFFER_SIZE = 128;
constexpr bool USE_DEVICE_VALIDATION = true;

// k-ary of b-plus trees, must be odd
constexpr unsigned int B_PLUS_TREE_K_ARY = 5;

// memory allocator appends a size in front of any buffer it allocates, this is its width in bytes
constexpr unsigned int ALLOCATOR_SIZE_HINT_WIDTH = 2;

// homogeneous allocator buffers (backend of allocator) embeds a pointer inside its allocated buffers;
// this buffer is
constexpr unsigned int ALLOCATOR_FREE_LIST_POINTER_WIDTH = sizeof(void *);

// change this if your CPU has a non-standard cache line
constexpr size_t ALLOCATOR_CACHELINE_SIZE = 64;

// width of the bitmask, limited by how big an integer type is
constexpr uint64_t ALLOCATOR_BITMASK_WIDTH = 64;

// type of bitmask needs to be a numeric type with size of ALLOCATOR_BITMASK_WIDTH
using AllocatorBitmaskType = uint64_t;

// type used to index into CacheFriendlyAllocator (will also become things like component ID if needed)
using AllocatorIndexType = int64_t;

// special value used to indicate the free list is exhausted
constexpr AllocatorIndexType INVALID_ALLOCATOR_INDEX = -1;

// initial capacity of the block list in CacheFriendlyAllocator
constexpr AllocatorIndexType ALLOCATOR_BLOCK_LIST_INITIAL_CAPACITY = 16;

} // namespace Configuration
} // namespace Core
} // namespace Engine
