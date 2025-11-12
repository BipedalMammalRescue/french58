#pragma once

#include "EngineCore/Pipeline/hash_id.h"
#include "EngineUtils/String/hex_strings.h"
#include "glm/ext/matrix_float2x2.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"

#include <spdlog/spdlog.h>
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


template<>
struct fmt::formatter<glm::vec2> : fmt::formatter<std::string>
{
    auto format(glm::vec2 my, format_context &ctx) const -> decltype(ctx.out())
    {
            return fmt::format_to(ctx.out(), "({}, {})", my.x, my.y);
    }
};

template<>
struct fmt::formatter<glm::vec3> : fmt::formatter<std::string>
{
    auto format(glm::vec3 my, format_context &ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "({}, {}, {})", my.x, my.y, my.z);
    }
};

template<>
struct fmt::formatter<glm::vec4> : fmt::formatter<std::string>
{
    auto format(glm::vec4 my, format_context &ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "({}, {}, {}, {})", my.x, my.y, my.z, my.w);
    }
};

template<>
struct fmt::formatter<glm::mat2> : fmt::formatter<std::string>
{
    auto format(glm::mat2 my, format_context &ctx) const -> decltype(ctx.out())
    {
            return fmt::format_to(ctx.out(), "({}, {})", my[0], my[1]);
    }
};

template<>
struct fmt::formatter<glm::mat3> : fmt::formatter<std::string>
{
    auto format(glm::mat3 my, format_context &ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "({}, {}, {})", my[0], my[1], my[2]);
    }
};

template<>
struct fmt::formatter<glm::mat4> : fmt::formatter<std::string>
{
    auto format(glm::mat4 my, format_context &ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "({}, {}, {}, {})", my[0], my[1], my[2], my[3]);
    }
};

template<>
struct fmt::formatter<Engine::Core::Pipeline::HashId> : fmt::formatter<std::string>
{
    auto format(Engine::Core::Pipeline::HashId my, format_context &ctx) const -> decltype(ctx.out())
    {
        char buf[33];
        buf[32] = 0;
        Engine::Utils::String::BinaryToHex(16, my.Hash.data(), buf);
        return fmt::format_to(ctx.out(), "<{}>", buf);
    }
};