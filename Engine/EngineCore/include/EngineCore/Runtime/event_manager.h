#pragma once

#include "EngineCore/Logging/logger.h"
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

// annotated events are event + author information
template <typename TEvent>
struct AnnotatedEvent
{
    TEvent Data;
    const char* AuthorSystem;
    int AuthorPath;
};

class EventManager 
{
private:
    friend class EventWriter;
    friend class GameLoop;

    struct EventSystemInstance
    {
        EventSystemDelegate Delegate;
        const char* Name;
    };

    int m_Registra = 0;
    std::vector<EventSystemInstance> m_Systems;
    Logging::Logger m_Logger;

    bool ExecuteAllSystems(ServiceTable* services, EventWriter& writer);

public:
    EventManager(Logging::LoggerService* loggerService);

    // input events needs to be registered so event systems can access them
    template <typename TEvent>
    EventOwner<TEvent> RegisterInputEvent(std::vector<AnnotatedEvent<TEvent>>* storage)
    {
        int id = m_Registra;
        m_Registra++;
        EventOwner<TEvent> owner;
        owner.m_ID = id;
        return owner;
    }

    // event systems are stateless functions executed that transforms input events to output events
    void RegisterEventSystem(EventSystemDelegate system, const char* displayName);
};

}