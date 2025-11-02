#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/module_assembly.h"
#include "EngineCore/Runtime/event_manager.h"
#include "EngineCore/Runtime/game_loop.h"
#include "ExampleGameplayModule/example_gameplay_module.h"
#include "EngineCore/Runtime/service_table.h"
#include "EngineCore/Runtime/module_manager.h"
#include "EngineCore/Runtime/event_writer.h"

#include <md5.h>

static Engine::Core::Pipeline::HashId ExampleModuleName = Engine::Extension::ExampleGameplayModule::GetDefinition().Name;

static bool s_ShouldRun = true;

bool FoobarEventSystem(const Engine::Core::Runtime::ServiceTable* services, Engine::Core::Runtime::EventWriter* writer)
{
    if (!s_ShouldRun)
        return false;

    s_ShouldRun = false;
    auto exampleModuleState = (Engine::Extension::ExampleGameplayModule::ModuleState*)services->ModuleManager->FindModule(ExampleModuleName);
    writer->WriteInputEvent<Engine::Extension::ExampleGameplayModule::YellEvent>(&exampleModuleState->YellOwner, { "your mom!!!" }, 0);
    return true;
}

int main()
{
    Engine::Core::Runtime::GameLoop gameloop(Engine::Core::Pipeline::ListModules());
    gameloop.AddEventSystem(&FoobarEventSystem, "FoobarEventSystem");
    return gameloop.Run(md5::compute("Entities/example.se_entity"));
}
