#include "ExampleGameplayModule/example_gameplay_module.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Pipeline/engine_callback.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/event_stream.h"

#include <EngineCore/Pipeline/module_definition.h>
#include <EngineCore/Runtime/service_table.h>

using namespace Engine;
using namespace Engine::Extension::ExampleGameplayModule;

static const char* LogChannels[] = { "ExampleGameplayModule" };

static void* InitModule(Core::Runtime::ServiceTable* services)
{
    // register event
    ModuleState* newState = new ModuleState();
    newState->Logger = services->LoggerService->CreateLogger(LogChannels, 1);
    newState->YellOwner = services->EventManager->RegisterInputEvent(&newState->YellEvents);

    return newState;
}

static void DisposeModule(Core::Runtime::ServiceTable *services, void *moduleState)
{
    ModuleState* state = static_cast<ModuleState*>(moduleState);
    delete state;
}

// TODO: need to fix the function header
static Core::Runtime::CallbackResult EventUpdate(const Core::Runtime::ServiceTable* services, void* moduleState, Core::Runtime::EventStream* events)
{
    ModuleState* state = static_cast<ModuleState*>(moduleState);

    while (events->MoveNext())
    {
        if (events->GetCurrentHeader().Owner != &state->YellOwner)
            continue;

        auto eventPtr = (YellEvent*)events->GetCurrentData();
        state->Logger.Information(eventPtr->Content);
    }

    return Core::Runtime::CallbackSuccess();
}

Core::Pipeline::ModuleDefinition Engine::Extension::ExampleGameplayModule::GetDefinition()
{
    static Core::Pipeline::EventCallback callbacks[] = {
        {
            EventUpdate
        }
    };

    return Core::Pipeline::ModuleDefinition 
    {
        md5::compute("ExampleGameplayModule"),
        InitModule,
        DisposeModule,
        nullptr,
        0,
        nullptr,
        0,
        callbacks,
        1,
        nullptr,
        0
    };
}