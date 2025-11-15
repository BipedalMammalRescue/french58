#include "RendererModule/Components/mesh_renderer.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/variant.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/service_table.h"
#include "EngineUtils/Memory/memstream_lite.h"
#include "RendererModule/renderer_module.h"
#include "EngineCore/Logging/logger_service.h"

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

Engine::Core::Runtime::CallbackResult Components::LoadMeshRenderer(size_t count, Utils::Memory::MemStreamLite& stream, Core::Runtime::ServiceTable* services, void* moduleState)
{
    RendererModuleState* state = static_cast<RendererModuleState*>(moduleState);

    Core::Logging::Logger logger = services->LoggerService->CreateLogger("MeshRendererLoader");

    struct _BoundStream{
        size_t Count;
        Utils::Memory::MemStreamLite* Stream;
    } boundStream = { count, &stream };

    // insert them into the state
    state->MeshRenderers.InsertRange<Utils::Memory::MemStreamLite>(count, &stream, [](MeshRenderer* buffer, size_t count, Utils::Memory::MemStreamLite* stream)
    {
        for (size_t i = 0; i < count; i++) 
        {
            int entity = stream->Read<int>();
            Core::Pipeline::HashId pipelineId = stream->Read<Core::Pipeline::HashId>();
            Core::Pipeline::HashId materialId = stream->Read<Core::Pipeline::HashId>();
            Core::Pipeline::HashId meshId = stream->Read<Core::Pipeline::HashId>();

            buffer[i] = {
                entity,
                pipelineId,
                materialId,
                meshId,
                nullptr,
                nullptr,
                0
            };
        }
    });

    return Core::Runtime::CallbackSuccess();
}