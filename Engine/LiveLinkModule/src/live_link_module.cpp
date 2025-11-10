#include "LiveLinkModule/live_link_module.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Pipeline/engine_callback.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/service_table.h"
#include "EngineCore/Logging/logger_service.h"
#include "LiveLinkModule/live_link_module_state.h"
#include "LiveLinkModule/live_link_session.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3_net/SDL_net.h"

using namespace Engine::Extension::LiveLinkModule;

static const char* LogChannels[] = { "LiveLinkModule" };

static void* InitializeModule(Engine::Core::Runtime::ServiceTable* services)
{
    Engine::Core::Logging::Logger logger = services->LoggerService->CreateLogger(LogChannels, 1);

    // initialize server
    NET_Server *server = NET_CreateServer(nullptr, 3459); // TODO: configuration system and configured ports
    if (server == nullptr)
    {
        logger.Error("Failed to create server, error: {error}", { SDL_GetError() });
        return nullptr;
    }

    auto state = new LiveLinkModuleState {
        logger,
        server
    };

    for (int i = 0; i < SDL_arraysize(state->Connections); i++)
    {
        state->Connections[i].Initialize(services, &state->Logger, i);
    }

    return state;
}

static void DisposeModule(Engine::Core::Runtime::ServiceTable* services, void* moduleState)
{
    delete static_cast<Engine::Extension::LiveLinkModule::LiveLinkModuleState*>(moduleState);
}

static Engine::Core::Runtime::CallbackResult PreupdateDelegate(Engine::Core::Runtime::ServiceTable* services, void* moduleState)
{
    auto state = static_cast<LiveLinkModuleState*>(moduleState);

    // check for connections
    NET_StreamSocket *candidate = NULL;

    while (true)
    {
        if (!NET_AcceptClient(state->Server, &candidate)) 
        {
            std::string error = "Error checking pending live link connections; details: ";
            error.append(SDL_GetError());
            return Engine::Core::Runtime::Crash(__FILE__, __LINE__, error);
        } 
        
        if (candidate == nullptr)
            break;

        // find an open slot
        int sessionSlot = -1;
        for (size_t i = 0; i < SDL_arraysize(state->Connections); i++)
        {
            if (!state->Connections[i].IsActive())
            {
                sessionSlot = i;
                break;
            }
        }

        if (sessionSlot < 0)
        {
            state->Logger.Warning("Maximum session count reached, dropping connection from {client}.", {NET_GetAddressString(NET_GetStreamSocketAddress(candidate))});
            NET_DestroyStreamSocket(candidate);
            continue;
        }
        else 
        {
            state->Logger.Information("Accepted connection at slot {slot} from {client}.", { sessionSlot, NET_GetAddressString(NET_GetStreamSocketAddress(candidate)) });
        }
        
        state->Connections[sessionSlot].Activate(candidate);
    }

    // try to read each connection (connection needs to be a state machine)
    for (LiveLinkSession& session : state->Connections)
    {
        if (!session.IsActive())
            continue;

        session.ReadToExhaustion();
    }

    return Engine::Core::Runtime::CallbackSuccess();
}

Engine::Core::Pipeline::ModuleDefinition Engine::Extension::LiveLinkModule::GetModuleDefinition()
{
    static const Core::Pipeline::SynchronousCallback synchronousCallbacks[] = {
        {
            Core::Pipeline::SynchronousCallbackStage::Preupdate,
            PreupdateDelegate
        }
    };

    return {
        .Name = HASH_NAME("LiveLinkModule"),
        .Initialize = InitializeModule,
        .Dispose = DisposeModule,
        .SynchronousCallbacks = synchronousCallbacks,
        .SynchronousCallbackCount = SDL_arraysize(synchronousCallbacks)
    };
}