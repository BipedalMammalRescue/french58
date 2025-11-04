#pragma once

#include "EngineCore/Logging/logger.h"
#include <complex>
#include <vector>

namespace Engine::Core::Logging {

class LoggerService;

}

namespace Engine::Core::Runtime {

class EventWriter;
struct ServiceTable;

using EventSystemDelegate = bool(*)(const ServiceTable* services, EventWriter* writer);

template <typename TEvent>
class EventOwner
{
private:
    friend class EventWriter;
    friend class EventManager;
    int m_ID = -1;
};

struct EventSystemInstance
{
    EventSystemDelegate Delegate;
    const char* Name;
};

class EventManager 
{
private:
    friend class EventWriter;
    friend class GameLoop;

    EventManager(Logging::LoggerService* loggerService);

    int m_Registra = 0;
    std::vector<EventSystemInstance> m_Systems;
    Logging::Logger m_Logger;

    bool ExecuteAllSystems(ServiceTable* services, EventWriter& writer);

    // event systems are stateless functions executed that transforms input events to output events
    void RegisterEventSystem(const EventSystemInstance* systems, size_t systemCount);

public:
    // input events needs to be registered so event systems can access them
    template <typename TEvent>
    EventOwner<TEvent> RegisterInputEvent()
    {
        int id = m_Registra;
        m_Registra++;
        EventOwner<TEvent> owner;
        owner.m_ID = id;
        return owner;
    }
};

}