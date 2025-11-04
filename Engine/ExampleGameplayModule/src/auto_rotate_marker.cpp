#include "ExampleGameplayModule/auto_rotate_marker.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "ExampleGameplayModule/example_gameplay_module.h"

using namespace Engine::Extension::ExampleGameplayModule;

bool Engine::Extension::ExampleGameplayModule::CompileMarker(Engine::Core::Pipeline::RawComponent input, std::ostream* output)
{
    output->write((char*)&input.Entity, sizeof(input.Entity));
    return true;
}

Engine::Core::Runtime::CallbackResult Engine::Extension::ExampleGameplayModule::LoadMarker(size_t count, std::istream* input, Engine::Core::Runtime::ServiceTable* services, void* moduleState)
{
    auto state = (ModuleState*) moduleState;

    for (size_t i = 0; i < count; i++)
    {
        int nextEntity = -1;
        input->read((char*)&nextEntity, sizeof(nextEntity));
        state->SpinningEntities.push_back(nextEntity);
    }

    return Core::Runtime::CallbackSuccess();
}