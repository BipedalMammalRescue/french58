#pragma once

#include <cstddef>
#include <cstdint>

namespace Engine::Core::Configuration {

constexpr long long STRING_LOAD_BUFFER_SIZE = 128;
constexpr bool USE_DEVICE_VALIDATION = true;

// initial stack size = 512mb
constexpr size_t INITIAL_STACK_SIZE = 512 * 1024 * 1024;

} // namespace Engine::Core::Configuration