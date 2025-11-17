#include "EngineCore/Ecs/Components/spatial_component.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/root_module.h"
#include "EngineUtils/Memory/memstream_lite.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "EngineCore/Pipeline/variant.h"
#include <md5.h>

using namespace Engine::Core::Ecs;

bool Components::CompileSpatialComponent(Core::Pipeline::RawComponent input, std::ostream* output) 
{
    // verify input shape
    if (input.FieldC != 3
        || input.FieldV[0].Payload.Type != Pipeline::VariantType::Vec3
        || input.FieldV[1].Payload.Type != Pipeline::VariantType::Vec3
        || input.FieldV[2].Payload.Type != Pipeline::VariantType::Vec3)
        return false;

    // get values from fields
    glm::vec3* translation = nullptr;
    glm::vec3* scale = nullptr;
    glm::vec3* angleRotation = nullptr;

    for (int i = 0; i < 3; i++) 
    {
        if (input.FieldV[i].Name == md5::compute("Translation"))
        {
            translation = &input.FieldV[i].Payload.Data.Vec3;
        }
        else if (input.FieldV[i].Name == md5::compute("Scale"))
        {
            scale = &input.FieldV[i].Payload.Data.Vec3;
        }
        else if (input.FieldV[i].Name == md5::compute("Rotation"))
        {
            angleRotation = &input.FieldV[i].Payload.Data.Vec3;
        }
    }

    if (translation == nullptr || scale == nullptr || angleRotation == nullptr)
        return false;

    // convert to quaternion
    glm::quat quatRotation(*angleRotation);

    // write out the data
    output->write((char*)&input.Entity, sizeof(int))
        .write((char*)translation, sizeof(glm::vec3))
        .write((char*)scale, sizeof(glm::vec3))
        .write((char*)&quatRotation, sizeof(glm::quat));
    return true;
}

Engine::Core::Runtime::CallbackResult Components::LoadSpatialComponent(size_t count, Utils::Memory::MemStreamLite& stream, Core::Runtime::ServiceTable* services, void* moduleState)
{
    Runtime::RootModuleState* state = static_cast<Runtime::RootModuleState*>(moduleState);
    state->SpatialComponents.reserve(state->SpatialComponents.size() + count);

    for (size_t i = 0; i < count; i++)
    {
        SpatialRelation newComponent;
        int ownerEntity = stream.Read<int>();
        newComponent.Translation = stream.Read<glm::vec3>();
        newComponent.Scale = stream.Read<glm::vec3>();
        newComponent.Rotation = stream.Read<glm::quat>();
        state->SpatialComponents[ownerEntity] = newComponent;
    }

    return Runtime::CallbackSuccess();
}

glm::mat4 Components::SpatialRelation::Transform() const
{
    // apply order: TRANSLATION * ROTATION * SCALE
    return glm::scale(glm::translate(glm::mat4(1.0), Translation) * glm::mat4_cast(Rotation), Scale);
}