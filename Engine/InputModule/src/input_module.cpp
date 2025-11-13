#include "InputModule/input_module.h"
#include "EngineCore/Logging/logger_service.h"
#include "EngineCore/Pipeline/name_pair.h"
#include "EngineCore/Runtime/input_manager.h"
#include "InputModule/Assets/input_action.h"

#include <EngineCore/Pipeline/asset_definition.h>
#include <EngineCore/Pipeline/engine_callback.h>
#include <EngineCore/Pipeline/module_definition.h>
#include <EngineCore/Runtime/crash_dump.h>
#include <EngineCore/Runtime/service_table.h>
#include <SDL3/SDL_scancode.h>
#include <md5.h>

using namespace Engine::Extension::InputModule;

InputModuleState::InputModuleState(Core::Runtime::ServiceTable* services)
    : Logger(services->LoggerService->CreateLogger("InputModule"))
{}


static void* InitInputModule(Engine::Core::Runtime::ServiceTable *services)
{
    return new InputModuleState(services);
}

static void DisposeInputModule(Engine::Core::Runtime::ServiceTable *services, void *moduleState)
{
    delete static_cast<InputModuleState*>(moduleState);
}

static Engine::Core::Runtime::CallbackResult PreupdateCallback(Engine::Core::Runtime::ServiceTable* services, void* moduleState)
{
    auto state = static_cast<InputModuleState*>(moduleState);

    state->DiscreteActivations.clear();
    state->DiscreteDeactivations.clear();

    for (SDL_Scancode scan : *services->InputManager->GetKeyDownEvents())
    {
        // check discrete action triggers
        auto foundDiscreteAction = state->DiscreteKeyboardTriggerTable.find(scan);
        if (foundDiscreteAction != state->DiscreteKeyboardTriggerTable.end() && foundDiscreteAction->second.CachedState == false)
        {
            foundDiscreteAction->second.CachedState = true;
            state->DiscreteActivations.push_back(foundDiscreteAction->second.Id);
        }

        // check emission action triggers
        auto foundEmissionAction = state->EmissionKeyboardTriggerTable.find(scan);
        if (foundEmissionAction != state->EmissionKeyboardTriggerTable.end())
        {
            auto foundEmission = state->Emissions.find(foundEmissionAction->second.Id);
            if (foundEmission != state->Emissions.end())
            {
                foundEmission->second = 1;
            }
        }
    }

    for (SDL_Scancode scan : *services->InputManager->GetKeyUpEvents())
    {
        // check discrete action triggers
        auto foundDiscreteAction = state->DiscreteKeyboardTriggerTable.find(scan);
        if (foundDiscreteAction != state->DiscreteKeyboardTriggerTable.end() && foundDiscreteAction->second.CachedState == true)
        {
            foundDiscreteAction->second.CachedState = false;
            state->DiscreteDeactivations.push_back(foundDiscreteAction->second.Id);
        }

        // check emission action triggers
        auto foundEmissionAction = state->EmissionKeyboardTriggerTable.find(scan);
        if (foundEmissionAction != state->EmissionKeyboardTriggerTable.end())
        {
            auto foundEmission = state->Emissions.find(foundEmissionAction->second.Id);
            if (foundEmission != state->Emissions.end())
            {
                foundEmission->second = 0;
            }
        }
    }

    return Engine::Core::Runtime::CallbackSuccess();
}

static Engine::Core::Runtime::CallbackResult PostupdateCallback(Engine::Core::Runtime::ServiceTable* services, void* moduleState)
{
    auto state = static_cast<InputModuleState*>(moduleState);
    
    state->DiscreteActivations.clear();
    state->DiscreteDeactivations.clear();

    return Engine::Core::Runtime::CallbackSuccess();
}

Engine::Core::Pipeline::ModuleDefinition Engine::Extension::InputModule::GetModuleDefinition()
{
    static const Core::Pipeline::AssetDefinition assets[] {
        {
            HASH_NAME("InputAction"),
            Assets::ContextualizeInputAction,
            Assets::IndexInputAction
        }
    };

    static const Core::Pipeline::SynchronousCallback synchronousCallbacks[] {
        {
            Core::Pipeline::SynchronousCallbackStage::Preupdate,
            PreupdateCallback
        },
        {
            Core::Pipeline::SynchronousCallbackStage::PostUpdate,
            PostupdateCallback
        }
    };

    return
    {
        HASH_NAME("InputModule"),
        InitInputModule,
        DisposeInputModule,
        assets,
        1,
        synchronousCallbacks,
        2,
        nullptr,
        0,
        nullptr,
        0
    };
}