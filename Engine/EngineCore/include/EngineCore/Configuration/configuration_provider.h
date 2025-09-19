#pragma once

#include <cstddef>

namespace Engine::Core::Configuration {

struct ConfigurationProvider
{
    int WindowHeight = 720;
    int WindowWidth = 960;

    long long STRING_LOAD_BUFFER_SIZE = 128;
    bool UseDeviceValidation = true;
    size_t INITIAL_STACK_SIZE = 512 * 1024 * 1024;
};

} // namespace Engine::Core::Configuration
