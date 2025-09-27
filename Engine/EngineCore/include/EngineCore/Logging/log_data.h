#pragma once

#include "EngineCore/Pipeline/variant.h"
namespace Engine::Core::Logging {

enum class LogLevel : unsigned char
{
    Verbose,
    Debug,
    Information,
    Warning,
    Error,
    Fatal
};

enum class LogEventType : unsigned char
{
    Header,
    Parameter,
    Terminator
};

struct LogHeader
{
    const char* Message;
    const char** Channels;
    size_t ChannelCount;
    size_t ThreadId;
    size_t Timestamp;
    LogLevel Level;
};

struct LogEvent
{
    union {
        LogHeader Header;
        Pipeline::Variant Parameter;
    } Payload;
    int Sequence;
    LogEventType Type;
};

}