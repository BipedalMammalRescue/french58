#pragma once

#include "EngineCore/Configuration/configuration_provider.h"
#include "EngineCore/Runtime/platform_access.h"
#include "EngineCore/Runtime/renderer_service.h"

namespace Engine::Core::Runtime {

class GameLoop
{
private:
    // services
    Configuration::ConfigurationProvider m_ConfigurationProvider;
    PlatformAccess m_PlatformAccess;
    RendererService m_RendererService;

public:
    GameLoop();
    int Run();
};

}