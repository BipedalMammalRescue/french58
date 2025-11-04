#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/module_assembly.h"
#include "EngineCore/Runtime/event_manager.h"
#include "EngineCore/Runtime/game_loop.h"
#include "EngineCore/Runtime/root_module.h"
#include "ExampleGameplayModule/example_gameplay_module.h"
#include "EngineCore/Runtime/service_table.h"
#include "EngineCore/Runtime/module_manager.h"
#include "EngineCore/Runtime/event_writer.h"
#include "glm/ext/scalar_constants.hpp"
#include "glm/ext/vector_float3.hpp"

#include <md5.h>

static Engine::Core::Pipeline::HashId ExampleModuleName = Engine::Extension::ExampleGameplayModule::GetDefinition().Name;

bool FoobarEventSystem(const Engine::Core::Runtime::ServiceTable* services, Engine::Core::Runtime::EventWriter* writer)
{
    if (!services->ModuleManager->GetRootModule()->TickEvent.has_value())
        return false;

    const Engine::Core::Runtime::RootModuleState* rootModule = services->ModuleManager->GetRootModule();

    float deltaTime = rootModule->TickEvent->DeltaTime;
    float rotationAngle = glm::pi<float>() / 1000 * deltaTime;

    Engine::Core::Ecs::Components::SpatialRelation newTransform;
    newTransform.Rotation = glm::quat(glm::vec3(0, rotationAngle, 0));
    newTransform.Translation = glm::vec3(0);
    newTransform.Scale = glm::vec3(1, 1, 1);

    auto exampleState = (Engine::Extension::ExampleGameplayModule::ModuleState*)services->ModuleManager->FindModule(
        ExampleModuleName
    );

    for (int entity : exampleState->SpinningEntities)
    {
        writer->WriteInputEvent(&rootModule->TransformUpdateEventOwner, Engine::Core::Runtime::TransformUpdateEvent { newTransform, entity }, 0);
    }

    return true;
}

int main()
{
    Engine::Core::Runtime::GameLoop gameloop(Engine::Core::Pipeline::ListModules(), {});
    gameloop.AddEventSystem(&FoobarEventSystem, "FoobarEventSystem");
    return gameloop.Run(md5::compute("Entities/example.se_entity"));
}
