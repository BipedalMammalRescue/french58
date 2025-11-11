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

LoggerService::LoggerService(Engine::Core::Configuration::ConfigurationProvider configs)
{
    auto formatter = std::make_unique<spdlog::pattern_formatter>();
    formatter->add_flag<SdlTimeFormatFlag>('E').set_pattern("[%E %^%l%$][%t][%n] %v");
    spdlog::set_formatter(std::move(formatter));
}

Logger LoggerService::CreateLogger(const char* channel)
{
    return Logger(spdlog::stdout_color_mt(channel));
}