#pragma once

#include "EngineCore/Configuration/configuration_provider.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/variant.h"
#include "EngineUtils/String/hex_strings.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float2.hpp"

#include <spdlog/spdlog.h>
#include <SDL3/SDL_thread.h>

namespace Engine::Core::Logging {

class LoggerService
{
public:
    LoggerService(Configuration::ConfigurationProvider configs);
    ~LoggerService() = default;

    Logger CreateLogger(const char* channel);
};

}

template<>
struct fmt::formatter<Engine::Core::Pipeline::Variant> : fmt::formatter<std::string>
{
    auto format(Engine::Core::Pipeline::Variant my, format_context &ctx) const -> decltype(ctx.out())
    {
        switch (my.Type)
        {
            case Engine::Core::Pipeline::VariantType::Byte:
                return fmt::format_to(ctx.out(), "{}", my.Data.Byte);
            case Engine::Core::Pipeline::VariantType::Bool:
                return fmt::format_to(ctx.out(), "{}", my.Data.Bool);
            case Engine::Core::Pipeline::VariantType::Int32:
                return fmt::format_to(ctx.out(), "{}", my.Data.Int32);
            case Engine::Core::Pipeline::VariantType::Uint32:
                return fmt::format_to(ctx.out(), "{}", my.Data.Uint32);
            case Engine::Core::Pipeline::VariantType::Float:
                return fmt::format_to(ctx.out(), "{}", my.Data.Float);
            case Engine::Core::Pipeline::VariantType::Vec2:
                return fmt::format_to(ctx.out(), "({}, {})", my.Data.Vec2.x, my.Data.Vec2.y);
            case Engine::Core::Pipeline::VariantType::Vec3:
                return fmt::format_to(ctx.out(), "({}, {}, {})", my.Data.Vec3.x, my.Data.Vec3.y, my.Data.Vec3.z);
            case Engine::Core::Pipeline::VariantType::Vec4:
                return fmt::format_to(ctx.out(), "({}, {}, {}, {})", my.Data.Vec4.x, my.Data.Vec4.y, my.Data.Vec4.z, my.Data.Vec4.w);
            case Engine::Core::Pipeline::VariantType::Mat2:
                return fmt::format_to(ctx.out(), "({}, {})", my.Data.Mat2[0], my.Data.Mat2[1]);
            case Engine::Core::Pipeline::VariantType::Mat3:
                return fmt::format_to(ctx.out(), "({}, {}, {})", my.Data.Mat3[0], my.Data.Mat3[1], my.Data.Mat3[2]);
            case Engine::Core::Pipeline::VariantType::Mat4:
                return fmt::format_to(ctx.out(), "({}, {}, {}, {})", my.Data.Mat4[0], my.Data.Mat4[1], my.Data.Mat4[2], my.Data.Mat4[3]);
            case Engine::Core::Pipeline::VariantType::Path:
                {
                    char buf[33];
                    buf[32] = 0;
                    Engine::Utils::String::BinaryToHex(16, my.Data.Path.Hash.data(), buf);
                    return fmt::format_to(ctx.out(), "{}", buf);
                }
            case Engine::Core::Pipeline::VariantType::Invalid:
                return fmt::format_to(ctx.out(), "");
        }
    }
};


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