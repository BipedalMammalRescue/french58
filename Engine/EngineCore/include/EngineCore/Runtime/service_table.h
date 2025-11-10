#pragma once

#include "EngineCore/Logging/logger_service.h"
#include "EngineCore/Runtime/input_manager.h"
namespace Engine::Core::Runtime {

class GraphicsLayer;
class WorldState;
class ModuleManager;
class EventManager;
class InputManager;
class NetworkLayer;

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
};

}