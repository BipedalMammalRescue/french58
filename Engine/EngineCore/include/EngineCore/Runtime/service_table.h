#pragma once

namespace Engine::Core::Runtime {

class GraphicsLayer;

// Table of services that should be accessed to modules.
struct ServiceTable 
{
    GraphicsLayer* GraphicsLayer;
};

}