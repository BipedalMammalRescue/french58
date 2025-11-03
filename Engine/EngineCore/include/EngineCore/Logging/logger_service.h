#pragma once

#include "EngineCore/Configuration/configuration_provider.h"
#include "EngineCore/Logging/log_data.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Runtime/crash_dump.h"

#include <SDL3/SDL_thread.h>
#include <concurrentqueue.h>

namespace Engine::Core::Runtime {
class GameLoop;
}

namespace Engine::Core::Logging {

class LoggerService
{
private:
    friend class Logger;
    friend class Runtime::GameLoop;

    std::atomic<int> m_Sequencer;
    moodycamel::ConcurrentQueue<LogEvent> m_Queue;
    SDL_Thread* m_Thread;
    LogLevel m_MinLevel;

    static int LoggerRoutine(void* state);
    Runtime::CallbackResult StartLogger();

    bool Write(const LogEvent& event);
    bool Write(const LogEvent&& event);
    int ReserveSequence(int segmentCount);

public:
    LoggerService(Configuration::ConfigurationProvider configs);
    ~LoggerService();
    Logger CreateLogger(const char** channels, size_t channelCount);
};

}