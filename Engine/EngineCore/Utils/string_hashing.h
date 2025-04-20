#pragma once

#include <cstddef>
#include <cstdint>

namespace Engine {
namespace Core {
namespace Utils {

uint64_t StringHash64(const char *string, size_t length);

uint64_t StringHash64(const char *string);

} // namespace Utils
} // namespace Core
} // namespace Engine
