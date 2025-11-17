#pragma once

#include "EngineCore/Logging/logger.h"
#include "EngineCore/Logging/logger_service.h"
#include "EngineCore/Runtime/crash_dump.h"
namespace Engine::Core::Runtime {

struct ServiceTable;

class NetworkLayer
{
private:
    Logging::Logger m_Logger;

public:
    NetworkLayer(Logging::LoggerService* loggerService);
    ~NetworkLayer();

    CallbackResult Initialize();
};

}