#include "EngineCore/Runtime/event_manager.h"
#include "EngineCore/Runtime/event_writer.h"

#include "EngineCore/Logging/logger_service.h"
#include "EngineCore/Runtime/service_table.h"

using namespace Engine::Core::Runtime;

static const char* LogChannels[] = { "EventManager" };

EventManager::EventManager(Engine::Core::Logging::LoggerService* loggerService)
    : m_Logger(loggerService->CreateLogger(LogChannels, 1))
{
}

void EventManager::RegisterEventSystem(const EventSystemInstance* systems, size_t systemCount)
{
    for (size_t i = 0; i < systemCount; i++)
    {
        m_Systems.push_back(systems[i]);
    }
}

bool EventManager::ExecuteAllSystems(ServiceTable* services, EventWriter& writer)
{
    bool hasEvent = false;

    writer.Initialize();

    for (const EventSystemInstance& system : m_Systems)
    {
        writer.m_UserName = system.Name;
        hasEvent |= system.Delegate(services, &writer);
    }

    return hasEvent;
}