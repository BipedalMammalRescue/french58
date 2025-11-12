#include "EngineCore/Ecs/Components/camera_component.h"
#include "EngineCore/Pipeline/variant.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/root_module.h"
#include "EngineUtils/Memory/memstream_lite.h"

using namespace Engine::Core;
using namespace Engine::Core::Ecs::Components;

bool Engine::Core::Ecs::Components::CompileCameraComponent(Pipeline::RawComponent input, std::ostream* output)
{
    // verify the only field
    if (input.FieldC != 1 || input.FieldV[0].Payload.Type != Pipeline::VariantType::Bool)
        return false;

    output->write((char*)&input.Entity, sizeof(int))
        .write((char*)&input.FieldV[0].Payload.Data.Bool, sizeof(bool));
    return true;
}

Runtime::CallbackResult Engine::Core::Ecs::Components::LoadCameraComponent(size_t count, Utils::Memory::MemStreamLite& stream, Runtime::ServiceTable* services, void* moduleState)
{
    Runtime::RootModuleState* state = static_cast<Runtime::RootModuleState*>(moduleState);
    state->CameraComponents.reserve(state->CameraComponents.size() + count);
    
    for (size_t i = 0; i < count; i++)
    {
        Camera newComponent;
        newComponent.Entity = stream.Read<int>();
        newComponent.IsPrimary = stream.Read<bool>();
        state->CameraComponents.push_back(newComponent);
    }

    return Runtime::CallbackSuccess();
}