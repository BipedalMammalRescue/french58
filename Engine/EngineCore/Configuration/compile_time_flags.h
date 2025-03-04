#pragma once

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

constexpr size_t ALLOCATOR_CACHELINE_SIZE = 64;
constexpr uint64_t ALLOCATOR_BITMASK_WIDTH = 64;
using AllocatorBitmaskType = uint64_t;
using AllocatorIndexType = int64_t;
constexpr AllocatorIndexType INVALID_ALLOCATOR_INDEX = -1;

} // namespace Configuration
} // namespace Core
} // namespace Engine
