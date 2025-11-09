#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/module_assembly.h"
#include "EngineCore/Runtime/event_manager.h"
#include "EngineCore/Runtime/game_loop.h"
#include "EngineCore/Runtime/root_module.h"
#include "ExampleGameplayModule/auto_rotate_marker.h"
#include "ExampleGameplayModule/example_gameplay_module.h"
#include "EngineCore/Runtime/service_table.h"
#include "EngineCore/Runtime/module_manager.h"
#include "EngineCore/Runtime/event_writer.h"
#include "InputModule/input_module.h"
#include "glm/ext/scalar_constants.hpp"
#include "glm/ext/vector_float3.hpp"

#include <md5.h>

static Engine::Core::Pipeline::HashId ExampleModuleName = Engine::Extension::ExampleGameplayModule::GetDefinition().Name.Hash;
static Engine::Core::Pipeline::HashId InputModuleName = Engine::Extension::InputModule::GetModuleDefinition().Name.Hash;

void FoobarEventSystem(const Engine::Core::Runtime::ServiceTable* services, void*, Engine::Core::Runtime::EventWriter* writer)
{
    if (!services->ModuleManager->GetRootModule()->TickEvent.has_value())
        return;

    const Engine::Core::Runtime::RootModuleState* rootModule = services->ModuleManager->GetRootModule();

    float delta = services->ModuleManager->GetRootModule()->TickEvent->DeltaTime;
    float rotationAngle = glm::pi<float>() / 1000 * delta;

    Engine::Core::Ecs::Components::SpatialRelation newTransform;
    newTransform.Translation = glm::vec3(0);
    newTransform.Scale = glm::vec3(1, 1, 1);

    auto exampleState = services->ModuleManager
        ->FindModule<Engine::Extension::ExampleGameplayModule::ModuleState>(ExampleModuleName);

    auto inputModuleState = services->ModuleManager->FindModule<Engine::Extension::InputModule::InputModuleState>(InputModuleName);

    for (const Engine::Extension::ExampleGameplayModule::RotationMarker& marker : exampleState->SpinningEntities)
    {
        auto foundEmission = inputModuleState->Emissions.find(marker.InputMethod);
        if (foundEmission == inputModuleState->Emissions.end())
            continue;
        
        float inputLevel = rotationAngle * foundEmission->second;
        newTransform.Rotation = glm::quat(glm::vec3(0, inputLevel, 0));

        if (inputLevel > 0)
        {
            writer->WriteInputEvent(&rootModule->TransformUpdateEventOwner, Engine::Core::Runtime::TransformUpdateEventData { newTransform, marker.Entity }, 0);
        }
    }

    return;
}

int main()
{
    Engine::Core::Runtime::GameLoop gameloop(Engine::Core::Pipeline::ListModules(), {});
    gameloop.AddEventSystem(&FoobarEventSystem, "FoobarEventSystem");
    return gameloop.Run(md5::compute("Entities/example.se_entity"));
}
