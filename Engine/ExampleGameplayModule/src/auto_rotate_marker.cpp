#include "ExampleGameplayModule/auto_rotate_marker.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/variant.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineUtils/Memory/memstream_lite.h"
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

Engine::Core::Runtime::CallbackResult Engine::Extension::ExampleGameplayModule::LoadMarker(size_t count, Utils::Memory::MemStreamLite& stream, Engine::Core::Runtime::ServiceTable* services, void* moduleState)
{
    auto state = (ModuleState*) moduleState;

    for (size_t i = 0; i < count; i++)
    {
        int nextEntity = stream.Read<int>();
        Core::Pipeline::HashId inputMethodId = stream.Read<Core::Pipeline::HashId>();
        state->SpinningEntities.push_back({ nextEntity, inputMethodId });
    }

    return Core::Runtime::CallbackSuccess();
}