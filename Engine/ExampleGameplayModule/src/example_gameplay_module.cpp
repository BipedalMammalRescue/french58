#include "ExampleGameplayModule/example_gameplay_module.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Pipeline/engine_callback.h"
#include "EngineCore/Runtime/crash_dump.h"

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

static Core::Runtime::CallbackResult EventUpdate(Core::Runtime::ServiceTable* services, void* moduleState)
{
    ModuleState* state = static_cast<ModuleState*>(moduleState);
    for (const Core::Runtime::AnnotatedEvent<YellEvent>& e : state->YellEvents)
    {
        state->Logger.Information(e.Data.Content);
    }

    return Core::Runtime::CallbackSuccess();
}

Core::Pipeline::ModuleDefinition Engine::Extension::ExampleGameplayModule::GetDefinition()
{
    Core::Pipeline::EngineCallback callbacks[] = {
        {
            Core::Pipeline::EngineCallbackStage::EventUpdate,
            &EventUpdate
        }
    };

    return Core::Pipeline::ModuleDefinition 
    {
        md5::compute("ExampleGameplayModule"),
        InitModule,
        DisposeModule,
        nullptr,
        0,
        callbacks,
        1,
        nullptr,
        0
    };
}