#include "ExampleGameplayModule/auto_rotate_marker.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/variant.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "ExampleGameplayModule/example_gameplay_module.h"

using namespace Engine::Extension::ExampleGameplayModule;

bool Engine::Extension::ExampleGameplayModule::CompileMarker(Engine::Core::Pipeline::RawComponent input, std::ostream* output)
{
    if (input.FieldC != 1 || input.FieldV->Payload.Type != Core::Pipeline::VariantType::Path)
        return false;

    output->write((char*)&input.Entity, sizeof(input.Entity));
    output->write((char*)input.FieldV[0].Payload.Data.Path.Hash.data(), sizeof(input.FieldV[0].Payload.Data.Path));
    return true;
}

Engine::Core::Runtime::CallbackResult Engine::Extension::ExampleGameplayModule::LoadMarker(size_t count, std::istream* input, Engine::Core::Runtime::ServiceTable* services, void* moduleState)
{
    auto state = (ModuleState*) moduleState;

    for (size_t i = 0; i < count; i++)
    {
        int nextEntity = -1;
        Core::Pipeline::HashId inputMethodId;
        
        input->read((char*)&nextEntity, sizeof(nextEntity));
        input->read((char*)&inputMethodId, sizeof(inputMethodId));

        state->SpinningEntities.push_back({ nextEntity, inputMethodId });
    }

    return Core::Runtime::CallbackSuccess();
}