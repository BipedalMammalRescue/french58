#include "LiveLinkModule/live_link_module.h"
#include "EngineCore/Runtime/service_table.h"

using namespace Engine::Extension::LiveLinkModule;

static void* InitializeModule(Engine::Core::Runtime::ServiceTable* services)
{
    auto state = new LiveLinkModuleState();
    return state;
}

static void DisposeModule(Engine::Core::Runtime::ServiceTable* services, void* moduleState)
{
    delete static_cast<Engine::Extension::LiveLinkModule::LiveLinkModuleState*>(moduleState);
}

Engine::Core::Pipeline::ModuleDefinition Engine::Extension::LiveLinkModule::GetModuleDefinition()
{
    return {
        HASH_NAME("LiveLinkModule"),
        InitializeModule,
        DisposeModule,
    };
}