#include "EngineCore/Configuration/configuration_provider.h"
#include "EngineCore/Logging/logger_service.h"
#include <thread>

using namespace Engine::Core::Logging;

int main()
{
    Engine::Core::Configuration::ConfigurationProvider configs;
    LoggerService logger(configs);

    for (int i = 0; i < 10; i++)
    {
        LogEvent event;

        const char* channels[] = {"Test", "App"};

        event.Type = Engine::Core::Logging::LogEventType::Header;
        event.Sequence = i;
        event.Payload.Header = {
            "Test message",
            channels,
            2,
            std::hash<std::thread::id>{}(std::this_thread::get_id()),
            LogLevel::Information
        };

        logger.Write(event);
    }

    LogEvent terminator;
    terminator.Type = LogEventType::Terminator;
    logger.Write(terminator);

    return 0;
}
