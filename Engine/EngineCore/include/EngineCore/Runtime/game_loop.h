#pragma once

#include "EngineCore/Configuration/configuration_provider.h"
#include "EngineCore/Runtime/graphics_layer.h"

namespace Engine::Core::Runtime {

class GameLoop
{
private:
    // services
    Configuration::ConfigurationProvider m_ConfigurationProvider;
    GraphicsLayer m_PlatformAccess;

public:
    GameLoop();
    int Run();
};

}