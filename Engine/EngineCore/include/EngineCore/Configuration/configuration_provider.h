#pragma once

#include "EngineCore/Logging/log_data.h"

#include <cstddef>

namespace Engine::Core::Configuration {

struct ConfigurationProvider
{
    uint32_t WindowHeight = 720;
    uint32_t WindowWidth = 960;

    long long STRING_LOAD_BUFFER_SIZE = 128;
    bool UseDeviceValidation = true;
    size_t INITIAL_STACK_SIZE = 512 * 1024 * 1024;

    int EntityVersion = 1;

    size_t LoggerBufferSize = 1024 * 32;
    Logging::LogLevel MinimumLogLevel = Logging::LogLevel::Information;

    size_t WorkerCount = 2;

    bool EnableVulkanValidationLayers = true;
};

constexpr size_t EntityLoadBatchSize = 1024;

} // namespace Engine::Core::Configuration
