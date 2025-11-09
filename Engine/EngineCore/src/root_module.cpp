#include "EngineCore/Runtime/root_module.h"
#include "EngineCore/Ecs/Components/camera_component.h"
#include "EngineCore/Ecs/Components/spatial_component.h"
#include "EngineCore/Pipeline/component_definition.h"
#include "EngineCore/Pipeline/engine_callback.h"
#include "EngineCore/Pipeline/module_definition.h"
#include "EngineCore/Pipeline/name_pair.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/event_writer.h"
#include "EngineCore/Runtime/service_table.h"
#include "EngineCore/Runtime/task_scheduler.h"
#include "EngineCore/Runtime/world_state.h"
#include "EngineCore/Scripting/api_data.h"
#include "EngineCore/Scripting/api_event.h"
#include "EngineCore/Scripting/api_query.h"
#include "EngineCore/Scripting/api_declaration.h"
#include "EngineCore/Scripting/event_declaration.h"
#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"

using namespace Engine::Core;
using namespace Engine::Core::Runtime;
using namespace Engine::Core::Ecs::Components;

bool CheckTickEventData(const ServiceTable* services, const void* moduleState, const void* data)
{
    if (data == nullptr)
        return false;

    auto state = static_cast<const RootModuleState*>(moduleState);
    if (!state->TickEvent.has_value())
        return false;
    return data == (const void*)&state->TickEvent.value();
}
DECLARE_SE_OBJECT(TickEventData, CheckTickEventData);


void RaiseTransformUpdateEvent(const ServiceTable* services, const void* moduleState, 
    const int* entity, 
    const glm::vec3* translation,
    const glm::vec3* scale,
    const glm::vec3* eulerRotation, 
    EventWriter* writer, int path)
{
    auto state = static_cast<const RootModuleState*>(moduleState);

    TransformUpdateEventData event {
        *translation,
        *scale,
        glm::quat(*eulerRotation)
    };

    writer->WriteInputEvent(&state->TransformUpdateEventOwner, event, path);
}
DECLARE_SE_EVENT_4(TransformUpdateEvent, int, glm::vec3, glm::vec3, glm::vec3, RaiseTransformUpdateEvent);


bool CheckTickEventDelegate(const ServiceTable* services, const void* moduleState)
{
    auto state = static_cast<const RootModuleState*>(moduleState);
    return state->TickEvent.has_value();
}
DECLARE_SE_API_0(CheckTickEvent, bool, CheckTickEventDelegate);


float GetTickTotalTimeDelegate(const ServiceTable* services, const void* moduleState)
{
    return services->WorldState->GetTotalTime();
}
DECLARE_SE_API_0(GetTotalTime, float, GetTickTotalTimeDelegate);


float GetTickDeltaTimeDelegate(const ServiceTable* services, const void* moduleState)
{
    return services->WorldState->GetDeltaTime();
}
DECLARE_SE_API_0(GetDeltaTime, float, GetTickDeltaTimeDelegate);


static void* InitializeRootModule(ServiceTable* services)
{
    auto newState = new RootModuleState();
    newState->TransformUpdateEventOwner = services->EventManager->RegisterInputEvent<TransformUpdateEventData>();
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

        auto data = (const TransformUpdateEventData*)eventStream.GetCurrentData();
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

    static const Scripting::ApiQueryBase* apiQueries[] {
        CheckTickEvent::GetQuery(),
        GetTotalTime::GetQuery(),
        GetDeltaTime::GetQuery()
    };

    static const Scripting::ApiEventBase* inputEvents[] {
        TransformUpdateEvent::GetEvent()
    };

    return Pipeline::ModuleDefinition {
        .Name = HASH_NAME("EngineRootModule"),
        .Initialize = InitializeRootModule,
        .Dispose = DisposeRootModule,
        .SynchronousCallbacks = synchronousCallbacks,
        .SynchronousCallbackCount = 2,
        .EventCallbacks = eventCallbacks,
        .EventCallbackCount = 1,
        .Components = rootComponents,
        .ComponentCount = sizeof(rootComponents) / sizeof(Pipeline::ComponentDefinition),
        .ApiQueries = apiQueries,
        .ApiQueryCount = sizeof(apiQueries) / sizeof(void*),
        .ApiEvents = inputEvents,
        .ApiEventCount = sizeof(inputEvents) / sizeof(void*)
    };
}