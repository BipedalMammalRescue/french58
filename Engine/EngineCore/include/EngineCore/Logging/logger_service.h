#pragma once

#include "EngineCore/Configuration/configuration_provider.h"
#include "EngineCore/Logging/log_data.h"
#include "EngineCore/Logging/logger.h"

#include <concurrentqueue.h>
#include <thread>

// TODO: how do I handle string data?

namespace Engine::Core::Logging {

class LoggerService
{
private:
    friend class Logger;
    std::atomic<int> m_Sequencer;
    moodycamel::ConcurrentQueue<LogEvent> m_Queue;
    std::thread m_Thread;
    LogLevel m_MinLevel;

    static void LoggerRoutine(moodycamel::ConcurrentQueue<LogEvent>* queue);
    bool Write(const LogEvent& event);
    bool Write(const LogEvent&& event);
    int ReserveSequence(int segmentCount);

public:
    LoggerService(Configuration::ConfigurationProvider configs);
    ~LoggerService();
    Logger CreateLogger(const char** channels, size_t channelCount);
};

}