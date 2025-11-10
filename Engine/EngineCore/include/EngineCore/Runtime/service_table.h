#pragma once

namespace Engine::Core::Logging {
class LoggerService;
}

namespace Engine::Core::Runtime {

class GraphicsLayer;
class WorldState;
class ModuleManager;
class EventManager;
class InputManager;
class NetworkLayer;
class TaskManager;

// Table of services that should be accessed to modules.
struct ServiceTable 
{
    Logging::LoggerService* LoggerService;
    GraphicsLayer* GraphicsLayer;
    WorldState* WorldState;
    ModuleManager* ModuleManager;
    EventManager* EventManager;
    InputManager* InputManager;
    NetworkLayer* NetworkLayer;
    TaskManager* TaskManager;
};

}