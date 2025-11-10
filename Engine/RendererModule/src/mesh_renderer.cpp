#include "RendererModule/Components/mesh_renderer.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/variant.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/service_table.h"
#include "RendererModule/renderer_module.h"
#include <EngineCore/Logging/logger_service.h>
#include <md5.h>

using namespace Engine::Extension::RendererModule;

bool Components::CompileMeshRenderer(Core::Pipeline::RawComponent input, std::ostream* output)
{
    // verify the structure is good
    if (input.FieldC != 3
        || input.FieldV[0].Payload.Type != Core::Pipeline::VariantType::Path 
        || input.FieldV[1].Payload.Type != Core::Pipeline::VariantType::Path
        || input.FieldV[2].Payload.Type != Core::Pipeline::VariantType::Path)
        return false;

    // write out the three asset references
    Core::Pipeline::HashId* pipelinePath = nullptr;
    Core::Pipeline::HashId* materialPath = nullptr;
    Core::Pipeline::HashId* meshPath = nullptr;
    for (int i = 0; i < 3; i++) 
    {
        if (input.FieldV[i].Name == md5::compute("Pipeline"))
        {
            pipelinePath = &input.FieldV[i].Payload.Data.Path;
        }
        else if (input.FieldV[i].Name == md5::compute("Material")) 
        {
            materialPath = &input.FieldV[i].Payload.Data.Path;
        }
        else if (input.FieldV[i].Name == md5::compute("Mesh")) 
        {
            meshPath = &input.FieldV[i].Payload.Data.Path;
        }

    }

    if (materialPath == nullptr || meshPath == nullptr || pipelinePath == nullptr)
        return false;

    output->write((char*)&input.Entity, sizeof(int));
    output->write((char*)pipelinePath, 16);
    output->write((char*)materialPath, 16);
    output->write((char*)meshPath, 16);

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
        Core::Pipeline::HashId pipelineId;
        Core::Pipeline::HashId materialId;
        Core::Pipeline::HashId meshId;

        input->read((char*)&entity, sizeof(int));
        input->read((char*)pipelineId.Hash.data(), 16);
        input->read((char*)materialId.Hash.data(), 16);
        input->read((char*)meshId.Hash.data(), 16);

        // find the mesh
        auto mesh = state->Meshes.find(meshId);
        if (mesh == state->Meshes.end())
        {
            logger.Error("Failed to load mesh renderer component for entity {entityId}, mesh ({meshId}) not found.", {entity, meshId});
            continue;
        }

        // find the material
        auto material = state->Materials.find(materialId);
        if (material == state->Materials.end())
        {
            logger.Error("Failed to load mesh renderer component for entity {entityId}, material ({materialId}) not found.", {entity, materialId});
            continue;
        }

        // find the pipeline
        auto pipelineLocation = state->PipelineIndex.find(pipelineId);
        if (pipelineLocation == state->PipelineIndex.end())
        {
            logger.Error("Failed to load mesh renderer component for entity {entityId}, pipeline ({pipelineId}) not found.", {entity, pipelineId});
            continue;
        }
        IndexedPipeline& pipeline = state->Pipelines[pipelineLocation->second];

        // verify the pipeline and material are compatible
        if (pipeline.Pipeline.PrototypeId != material->second.PrototypeId)
        {
            logger.Error("Failed to load mesh renderer component for entity {entityId}, pipeline prototype ({pipelinePrototype}) and material prototype ({materialPrototype}) do not match.", {entity, pipeline.Pipeline.PrototypeId, material->second.PrototypeId});
            continue;
        }

        // add mesh to pipeline
        pipeline.Objects.push_back({ entity, mesh->second, material->second });
    }

    return Core::Runtime::CallbackSuccess();
}