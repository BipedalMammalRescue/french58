#pragma once

namespace Engine::Core::Runtime {

class GraphicsLayer;
class WorldState;
class ModuleManager;

// Table of services that should be accessed to modules.
struct ServiceTable 
{
    GraphicsLayer* GraphicsLayer;
    WorldState* WorldState;
    ModuleManager* ModuleManager;
};

}