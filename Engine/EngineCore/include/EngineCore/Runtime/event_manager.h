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

    std::vector<void*> m_EventStorageList;
    std::vector<EventSystemInstance> m_Systems;
    Logging::Logger m_Logger;

public:
    EventManager(Logging::LoggerService* loggerService);

    // input events needs to be registered so event systems can access them
    template <typename TEvent>
    EventOwner<TEvent> RegisterInputEvent(std::vector<AnnotatedEvent<TEvent>>* storage)
    {
        int id = m_EventStorageList.size();
        m_EventStorageList.push_back(storage);
        EventOwner<TEvent> owner;
        owner.m_ID = id;
        return owner;
    }

    // event systems are stateless functions executed that transforms input events to output events
    void RegisterEventSystem(EventSystemDelegate system, const char* displayName);

    bool ExecuteAllSystems(ServiceTable* services);
};

class EventWriter
{
private:
    friend class EventManager;
    
    EventManager* m_Manager;
    const char* m_UserName;

    EventWriter(EventManager* manager) : m_Manager(manager), m_UserName(nullptr) {}

public:
    template <typename TEvent>
    void WriteInputEvent(EventOwner<TEvent>* owner, TEvent eventData, int authorPath)
    {
        // TODO: temporary implementation, when the engine goes multi-threaded we'll need thread-local storage for the event writers; it'll be fairly easy to generate some headers and just store everything in a binary stream
        std::vector<AnnotatedEvent<TEvent>>* storage = static_cast<std::vector<AnnotatedEvent<TEvent>>*>(m_Manager->m_EventStorageList[owner->m_ID]);
        storage->push_back({ eventData, m_UserName, authorPath });
    }
};

}