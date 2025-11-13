#include "EngineCore/Logging/logger_service.h"
#include "EngineCore/Configuration/configuration_provider.h"
#include "SDL3/SDL_timer.h"
#include "spdlog/common.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/pattern_formatter.h"
#include <string>

using namespace Engine::Core::Logging;

static char Get10s(size_t input)
{
    return ((input - (input / 100)) / 10) % 10 + '0';
}

static char Get100s(size_t input)
{
    return ((input - (input / 1000)) / 100) % 10 + '0';
}

static char Get1s(size_t input)
{
    return (input - (input / 10)) % 10 + '0';
}

class SdlTimeFormatFlag : public spdlog::custom_flag_formatter
{
public:
    void format(const spdlog::details::log_msg & msg, const std::tm &, spdlog::memory_buf_t &dest) override
    {
        // [%02lu:%02lu:%02lu.%03lu ", hours, minutes, seconds, ms
        auto ticks = SDL_GetTicks();

        size_t hours = ticks / 1000 / 60 / 60;
        size_t minutes = (ticks / 1000 / 60) % 60;
        size_t seconds = (ticks / 1000) % 60;
        size_t ms = ticks % 1000;

        char buf[] = {
            Get10s(hours),
            Get1s(hours),
            ':',
            Get10s(minutes),
            Get1s(minutes),
            ':',
            Get10s(seconds),
            Get1s(seconds),
            '.',
            Get100s(ms),
            Get10s(ms),
            Get1s(ms),
            0
        };
        dest.append(buf, buf + sizeof(buf));
    }

    std::unique_ptr<custom_flag_formatter> clone() const override
    {
        return spdlog::details::make_unique<SdlTimeFormatFlag>();
    }
};

class SerilogLevelFormatFlag : public spdlog::custom_flag_formatter
{
public:
    void format(const spdlog::details::log_msg & msg, const std::tm &, spdlog::memory_buf_t &dest) override
    {
        const char ver[] = "VER";
        const char dbg[] = "DBG";
        const char inf[] = "INF";
        const char wrn[] = "WRN";
        const char err[] = "ERR";
        const char ftl[] = "FTL";

        const char* level = nullptr;

        switch (msg.level)
        {
        case spdlog::level::trace:
            level = ver;
            break;
        case spdlog::level::debug:
            level = dbg;
            break;
        case spdlog::level::info:
            level = inf;
            break;
        case spdlog::level::warn:
            level = wrn;
            break;
        case spdlog::level::err:
            level = err;
            break;
        case spdlog::level::critical:
            level = ftl;
            break;
        case spdlog::level::off:
        case spdlog::level::n_levels:
            break;
        }

        if (level != nullptr)
        {
            dest.append(level, level + sizeof(ver));
        }
    }

    std::unique_ptr<custom_flag_formatter> clone() const override
    {
        return spdlog::details::make_unique<SerilogLevelFormatFlag>();
    }
};


LoggerService::LoggerService(Engine::Core::Configuration::ConfigurationProvider configs)
{
    auto formatter = std::make_unique<spdlog::pattern_formatter>();
    formatter->add_flag<SdlTimeFormatFlag>('E').add_flag<SerilogLevelFormatFlag>('L').set_pattern("[%E %^%L%$][%t][%n] %v");
    spdlog::set_formatter(std::move(formatter));
}

Logger LoggerService::CreateLogger(const char* channel)
{
    return Logger(spdlog::stdout_color_mt(channel));
}