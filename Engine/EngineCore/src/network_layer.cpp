#include "EngineCore/Runtime/network_layer.h"
#include "EngineCore/Logging/logger_service.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/service_table.h"
#include "SDL3_net/SDL_net.h"

using namespace Engine::Core::Runtime;

static const char* LogChannels[] = { "NetworkLayer" };

NetworkLayer::NetworkLayer(Engine::Core::Logging::LoggerService* loggerService)
{
    m_Logger = loggerService->CreateLogger(LogChannels, 1);
}

NetworkLayer::~NetworkLayer()
{
    NET_Quit();
}

CallbackResult NetworkLayer::Initialize()
{
    // initialize sdl_net
    if (!NET_Init()) 
        return Crash(__FILE__, __LINE__, std::string("Failed to initialize SDL_net, error:") + SDL_GetError());

    return CallbackSuccess();
}