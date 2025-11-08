#include "EngineCore/Runtime/root_module.h"
#include "EngineCore/Ecs/Components/camera_component.h"
#include "EngineCore/Ecs/Components/spatial_component.h"
#include "EngineCore/Pipeline/component_definition.h"
#include "EngineCore/Pipeline/engine_callback.h"
#include "EngineCore/Pipeline/module_definition.h"
#include "EngineCore/Pipeline/name_pair.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/service_table.h"
#include "EngineCore/Runtime/task_scheduler.h"
#include "EngineCore/Runtime/world_state.h"

using namespace Engine::Core;
using namespace Engine::Core::Runtime;
using namespace Engine::Core::Ecs::Components;

static void* InitializeRootModule(ServiceTable* services)
{
    auto newState = new RootModuleState();
    newState->TransformUpdateEventOwner = services->EventManager->RegisterInputEvent<TransformUpdateEvent>();
    return newState;
}

static void DisposeRootModule(ServiceTable* services, void* moduleState) 
{
    delete static_cast<RootModuleState*>(moduleState);
}

static Runtime::CallbackResult EventCallback(const Runtime::ServiceTable* services, ITaskScheduler* scheduler, void* moduleState, Runtime::EventStream eventStream)
{
    auto state = static_cast<RootModuleState*>(moduleState);

    while (eventStream.MoveNext())
    {
        if (eventStream.GetCurrentHeader().Owner != &state->TransformUpdateEventOwner)
            continue;

        auto data = (const TransformUpdateEvent*)eventStream.GetCurrentData();
        auto foundTransform = state->SpatialComponents.find(data->EntityId);
        if (foundTransform == state->SpatialComponents.end())
            continue;

        // apply change
        foundTransform->second.Translation += data->NewTransform.Translation;
        foundTransform->second.Scale *= data->NewTransform.Scale;
        foundTransform->second.Rotation *= data->NewTransform.Rotation;
    }

    return CallbackSuccess();
}

static Runtime::CallbackResult PreupdateCallback(Runtime::ServiceTable* services, void* moduleState)
{
    auto state = static_cast<RootModuleState*>(moduleState);

    state->TickEvent = {
        services->WorldState->GetTotalTime(),
        services->WorldState->GetDeltaTime()
    };

    return CallbackSuccess();
}

static Runtime::CallbackResult PostupdateCallback(Runtime::ServiceTable* services, void* moduleState)
{
    auto state = static_cast<RootModuleState*>(moduleState);

    // clear existing output events
    state->TickEvent.reset();

    return CallbackSuccess();
}

Pipeline::ModuleDefinition RootModuleState::GetDefinition() 
{
    static const Pipeline::ComponentDefinition rootComponents[] {
        {
            HASH_NAME("SpatialRelation"),
            Ecs::Components::CompileSpatialComponent,
            Ecs::Components::LoadSpatialComponent
        },
        {
            HASH_NAME("Camera"),
            Ecs::Components::CompileCameraComponent,
            Ecs::Components::LoadCameraComponent
        }
    };

    static const Pipeline::EventCallback eventCallbacks[] {
        {
            EventCallback
        }
    };

    static const Pipeline::SynchronousCallback synchronousCallbacks[] {
        {
            Pipeline::SynchronousCallbackStage::Preupdate,
            PreupdateCallback
        },
        {
            Pipeline::SynchronousCallbackStage::PostUpdate,
            PostupdateCallback
        }
    };

    return Pipeline::ModuleDefinition {
        HASH_NAME("EngineRootModule"),
        InitializeRootModule,
        DisposeRootModule,
        nullptr,
        0,
        synchronousCallbacks,
        2,
        eventCallbacks,
        1,
        rootComponents,
        sizeof(rootComponents) / sizeof(Pipeline::ComponentDefinition)
    };
}