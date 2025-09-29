#include "EngineCore/Logging/logger.h"
#include "EngineCore/Logging/log_data.h"
#include "EngineCore/Logging/logger_service.h"

#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_thread.h>

using namespace Engine::Core::Logging;

// break the log message into 1 + N normalized segments and send them with distinct sequences
bool Logger::Write(LogLevel level, const char* message, std::initializer_list<Pipeline::Variant> params)
{
    if (level < Service->m_MinLevel)
        return true;

    int paramCount = params.size();
    int seq = Service->ReserveSequence(paramCount + 1);

    LogEvent event {
        {.Header= {
            message,
            Channels,
            ChanneCount,
            SDL_GetCurrentThreadID(),
            SDL_GetTicks(),
            paramCount,
            level
        }},
        seq,
        LogEventType::Header
    };

    if (!Service->Write(event))
        return false;

    for (int i = 0; i < paramCount; i++)
    {
        LogEvent subEvent {
            {.Parameter=params.begin()[i]},
            seq + i + 1,
            LogEventType::Parameter
        };

        if (!Service->Write(subEvent))
            return false;
    }

    return true;
}