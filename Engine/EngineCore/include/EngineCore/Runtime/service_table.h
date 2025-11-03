#pragma once

#include "EngineCore/Logging/logger_service.h"
namespace Engine::Core::Runtime {

class GraphicsLayer;
class WorldState;
class ModuleManager;
class EventManager;

// Table of services that should be accessed to modules.
struct ServiceTable 
{
    Logging::LoggerService* LoggerService;
    GraphicsLayer* GraphicsLayer;
    WorldState* WorldState;
    ModuleManager* ModuleManager;
    EventManager* EventManager;
};

}