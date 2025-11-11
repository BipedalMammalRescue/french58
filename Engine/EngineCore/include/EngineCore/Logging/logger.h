#pragma once

#include "spdlog/spdlog.h"

#include <memory>

namespace Engine::Core::Logging {

class LoggerService;

class Logger
{
    std::shared_ptr<spdlog::logger> m_CoreLogger;

public:
    Logger(std::shared_ptr<spdlog::logger> coreLogger) : m_CoreLogger(coreLogger) {}

    template <typename... Args>
    inline void Verbose(const char* message, Args &&...args)
    {
        m_CoreLogger->trace(message, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline void Debug(const char* message, Args &&...args)
    {
        m_CoreLogger->debug(message, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline void Information(const char* message, Args &&...args)
    {
        m_CoreLogger->info(message, std::forward<Args>(args)...);

    }

    template <typename... Args>
    inline void Warning(const char* message, Args &&...args)
    {
        m_CoreLogger->warn(message, std::forward<Args>(args)...);

    }

    template <typename... Args>
    inline void Error(const char* message, Args &&...args)
    {
        m_CoreLogger->error(message, std::forward<Args>(args)...);

    }

    template <typename... Args>
    inline void Fatal(const char* message, Args &&...args)
    {
        m_CoreLogger->critical(message, std::forward<Args>(args)...);
    }
};

}