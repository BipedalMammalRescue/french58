#pragma once

#include "EngineCore/Logging/log_data.h"

#include <cstddef>
#include <initializer_list>

namespace Engine::Core::Logging {

class LoggerService;

struct Logger
{
    const char** Channels;
    size_t ChanneCount;
    LoggerService* Service;

    bool Write(LogLevel level, const char* message, std::initializer_list<Pipeline::Variant> params);

    inline bool Verbose(const char* message, std::initializer_list<Pipeline::Variant> params = {})
    {
        return Write(LogLevel::Verbose, message, params);
    }

    inline bool Debug(const char* message, std::initializer_list<Pipeline::Variant> params = {})
    {
        return Write(LogLevel::Debug, message, params);
    }

    inline bool Information(const char* message, std::initializer_list<Pipeline::Variant> params = {})
    {
        return Write(LogLevel::Information, message, params);
    }

    inline bool Warning(const char* message, std::initializer_list<Pipeline::Variant> params = {})
    {
        return Write(LogLevel::Warning, message, params);
    }

    inline bool Error(const char* message, std::initializer_list<Pipeline::Variant> params = {})
    {
        return Write(LogLevel::Error, message, params);
    }

    inline bool Fatal(const char* message, std::initializer_list<Pipeline::Variant> params = {})
    {
        return Write(LogLevel::Fatal, message, params);
    }
};

}