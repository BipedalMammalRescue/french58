#include "EngineCore/Logging/logger.h"
#include "EngineCore/Logging/log_data.h"
#include "EngineCore/Logging/logger_service.h"

#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_thread.h>

using namespace Engine::Core::Logging;

static size_t GetTimestamp()
{
    return SDL_GetTicks();
}

bool Logger::Write(LogLevel level, const char* message, std::initializer_list<Pipeline::Variant> params)
{
    if (level < Service->m_MinLevel)
        return true;

    int seq = Service->GetSequence();

    LogEvent event {
        {.Header= {
            message,
            Channels,
            ChanneCount,
            SDL_GetCurrentThreadID(),
            GetTimestamp(),
            level
        }},
        seq,
        LogEventType::Header
    };

    if (!Service->Write(event))
        return false;

    for (auto i = params.begin(); i != params.end(); i++)
    {
        LogEvent subEvent {
            {.Parameter=*i},
            seq,
            LogEventType::Parameter
        };

        if (!Service->Write(subEvent))
            return false;
    }

    return true;
}