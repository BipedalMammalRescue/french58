#include "RendererModule/Components/mesh_renderer.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/variant.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/service_table.h"
#include "RendererModule/renderer_module.h"
#include <md5.h>

using namespace Engine::Extension::RendererModule;

bool Components::CompileMeshRenderer(Core::Pipeline::RawComponent input, std::ostream* output)
{
    // verify the structure is good
    if (input.FieldC != 2
        || input.FieldV[0].Payload.Type != Core::Pipeline::VariantType::Path 
        || input.FieldV[1].Payload.Type != Core::Pipeline::VariantType::Path)
        return false;

    // write out the two asset references to the output
    std::array<unsigned char, 16>* materialPath = nullptr;
    std::array<unsigned char, 16>* meshPath = nullptr;
    for (int i = 0; i < 2; i++) 
    {
        if (input.FieldV[i].Name == md5::compute("Material")) 
        {
            materialPath = &input.FieldV[i].Payload.Data.Path;
        }
        if (input.FieldV[i].Name == md5::compute("Mesh")) 
        {
            meshPath = &input.FieldV[i].Payload.Data.Path;
        }
    }

    if (materialPath == nullptr || meshPath == nullptr)
        return false;

    output->write((char*)&input.Entity, sizeof(int));
    output->write((char*)materialPath->data(), 16);
    output->write((char*)meshPath->data(), 16);
    return true;
}

Engine::Core::Runtime::CallbackResult Components::LoadMeshRenderer(size_t count, std::istream* input, Core::Runtime::ServiceTable* services, void* moduleState)
{
    ModuleState* state = static_cast<ModuleState*>(moduleState);

    const char* logChannels[] = {"MeshRendererLoader"};
    Core::Logging::Logger logger = services->LoggerService->CreateLogger(logChannels, 1);

    for (size_t i = 0; i < count; i++) 
    {
        int entity;
        Core::Pipeline::HashId materialId;
        Core::Pipeline::HashId mesh;

        input->read((char*)&entity, sizeof(int));
        input->read((char*)materialId.Hash.data(), 16);
        input->read((char*)mesh.Hash.data(), 16);

        // find the material
        auto materialLocation = state->MaterialIndex.find(materialId);
        if (materialLocation == state->MaterialIndex.end())
        {
            // TODO: need some more readable notation for mesh renderer component locations
            logger.Error("Failed to load mesh renderer component for entity {entityId}, material ({materialId}) not found.", {entity, materialId});
            continue;
        }

        // add mesh renderer to the material
        IndexedMaterial& material = state->Pipelines[materialLocation->second.Pipeline].Materials[materialLocation->second.Material];
        material.Meshes.push_back({ entity, mesh });
    }

    return Core::Runtime::CallbackSuccess();
}