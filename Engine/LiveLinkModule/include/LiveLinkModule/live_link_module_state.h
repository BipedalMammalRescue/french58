#pragma once

#include "EngineCore/Logging/logger.h"
#include "LiveLinkModule/live_link_session.h"
#include "SDL3_net/SDL_net.h"
namespace Engine::Extension::LiveLinkModule {

struct LiveLinkModuleState
{
    Core::Logging::Logger Logger;
    NET_Server* Server;
    LiveLinkSession Connections[16];
};

}