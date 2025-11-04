#include "ExampleGameplayModule/example_gameplay_module.h"
#include "EngineCore/Runtime/task_scheduler.h"
#include "ExampleGameplayModule/auto_rotate_marker.h"

#include <EngineCore/Logging/logger.h>
#include <EngineCore/Pipeline/component_definition.h>
#include <EngineCore/Pipeline/engine_callback.h>
#include <EngineCore/Runtime/crash_dump.h>
#include <EngineCore/Runtime/event_stream.h>
#include <EngineCore/Pipeline/module_definition.h>
#include <EngineCore/Runtime/service_table.h>

#include <md5.h>

using namespace Engine;
using namespace Engine::Extension::ExampleGameplayModule;

static const char* LogChannels[] = { "ExampleGameplayModule" };

static void* InitModule(Core::Runtime::ServiceTable* services)
{
    // register event
    ModuleState* newState = new ModuleState();
    newState->Logger = services->LoggerService->CreateLogger(LogChannels, 1);
    newState->YellOwner = services->EventManager->RegisterInputEvent<YellEvent>();

    return newState;
}

static void DisposeModule(Core::Runtime::ServiceTable *services, void *moduleState)
{
    ModuleState* state = static_cast<ModuleState*>(moduleState);
    delete state;
}

static Core::Runtime::CallbackResult EventUpdate(const Core::Runtime::ServiceTable* services, Core::Runtime::ITaskScheduler* scheduler, void* moduleState, Core::Runtime::EventStream events)
{
    ModuleState* state = static_cast<ModuleState*>(moduleState);

    while (events.MoveNext())
    {
        if (events.GetCurrentHeader().Owner != &state->YellOwner)
            continue;

        auto eventPtr = (YellEvent*)events.GetCurrentData();
        state->Logger.Information(eventPtr->Content);
    }

    return Core::Runtime::CallbackSuccess();
}

Core::Pipeline::ModuleDefinition Engine::Extension::ExampleGameplayModule::GetDefinition()
{
    static Core::Pipeline::EventCallback callbacks[] {
        {
            EventUpdate
        }
    };

    static Core::Pipeline::ComponentDefinition componentDefinitions[] {
        {
            md5::compute("AutoRotateMarker"),
            CompileMarker,
            LoadMarker
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
        componentDefinitions,
        1
    };
}