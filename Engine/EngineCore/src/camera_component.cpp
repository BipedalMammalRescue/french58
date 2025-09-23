#include "EngineCore/Ecs/Components/camera_component.h"
#include "EngineCore/Pipeline/variant.h"
#include "EngineCore/Runtime/root_module.h"

using namespace Engine::Core;
using namespace Engine::Core::Ecs::Components;

bool CompileCameraComponent(Pipeline::RawComponent input, std::ostream* output)
{
    // verify the only field
    if (input.FieldC != 1 || input.FieldV[0].Payload->Type != Pipeline::VariantType::Bool)
        return false;

    output->write((char*)&input.Entity, sizeof(int))
        .write((char*)&input.FieldV[0].Payload->Data.Bool, sizeof(bool));
    return true;
}

void LoadCameraComponent(size_t count, std::istream* input, Runtime::ServiceTable* services, void* moduleState)
{
    Runtime::RootModuleState* state = static_cast<Runtime::RootModuleState*>(moduleState);
    state->CameraComponents.reserve(state->CameraComponents.size() + count);
    
    for (size_t i = 0; i < count; i++)
    {
        Camera newComponent;
        input->read((char*)&newComponent.Entity, sizeof(int))
            .read((char*)&newComponent.IsPrimary, sizeof(bool));
        state->CameraComponents.push_back(newComponent);
    }
}