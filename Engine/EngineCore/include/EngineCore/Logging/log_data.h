#pragma once

#include "EngineCore/Pipeline/variant.h"
#include "glm/ext/vector_float2.hpp"
#include <cstdint>

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
    int ParamCount;
    LogLevel Level;
};

enum class LogParameterType
{
    FixedSize,
    String
};

struct LogParameter 
{
    LogParameterType Type;
    union {
        const char* String;
        Pipeline::Variant Fixed;
    } Data;

    LogParameter() = default;

    LogParameter(const Pipeline::Variant& other) : Type(LogParameterType::FixedSize)
    {
        Data.Fixed = other;
    }

    LogParameter(const Pipeline::Variant&& other) : Type(LogParameterType::FixedSize)
    {
        Data.Fixed = other;
    }

    LogParameter(const char* other) : Type(LogParameterType::String)
    {
        Data.String = other;
    }
    
    LogParameter(const unsigned char& byte) : 
        Type(LogParameterType::FixedSize), Data({.Fixed = byte}){}
    LogParameter(const bool &variant)
        : Type(LogParameterType ::FixedSize), Data({.Fixed = variant}) {}
    LogParameter(const int32_t &variant)
        : Type(LogParameterType ::FixedSize), Data({.Fixed = variant}) {}
    LogParameter(const uint32_t &variant)
        : Type(LogParameterType ::FixedSize), Data({.Fixed = variant}) {}
    LogParameter(const float &variant)
        : Type(LogParameterType ::FixedSize), Data({.Fixed = variant}) {}
    LogParameter(const glm ::vec2 &variant)
        : Type(LogParameterType ::FixedSize), Data({.Fixed = variant}) {}
    LogParameter(const glm ::vec3 &variant)
        : Type(LogParameterType ::FixedSize), Data({.Fixed = variant}) {}
    LogParameter(const glm ::vec4 &variant)
        : Type(LogParameterType ::FixedSize), Data({.Fixed = variant}) {}
    LogParameter(const glm ::mat2 &variant)
        : Type(LogParameterType ::FixedSize), Data({.Fixed = variant}) {}
    LogParameter(const glm ::mat3 &variant)
        : Type(LogParameterType ::FixedSize), Data({.Fixed = variant}) {}
    LogParameter(const glm ::mat4 &variant)
        : Type(LogParameterType ::FixedSize), Data({.Fixed = variant}) {}
    LogParameter(const Pipeline ::HashId &variant)
        : Type(LogParameterType ::FixedSize), Data({.Fixed = variant}) {}
};

struct LogEvent
{
    union {
        LogHeader Header;
        LogParameter Parameter;
    } Payload;
    int Sequence;
    LogEventType Type;
};

}