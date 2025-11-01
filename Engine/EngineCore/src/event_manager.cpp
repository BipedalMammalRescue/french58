#include "EngineCore/Runtime/event_manager.h"

#include "EngineCore/Logging/logger_service.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/service_table.h"

using namespace Engine::Core::Runtime;

const char* LogChannels[] = { "EventManager" };

EventManager::EventManager(Engine::Core::Logging::LoggerService* loggerService)
    : m_Logger(loggerService->CreateLogger(LogChannels, 1))
{
}

void EventManager::RegisterEventSystem(EventSystemDelegate system, const char* displayName)
{
    m_Systems.push_back({system, displayName});
}

CallbackResult EventManager::ExecuteAllSystems(ServiceTable* services)
{
    EventWriter writer(this);

    for (const EventSystemInstance& system : m_Systems)
    {
        writer.m_UserName = system.Name;
        system.Delegate(services, &writer);
    }

    return CallbackSuccess();
}