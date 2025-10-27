#include "RendererModule/Assets/material.h"
#include "EngineCore/Logging/logger.h"
#include "RendererModule/renderer_module.h"
#include "RendererModule/Assets/material.h"

#include <EngineCore/Pipeline/hash_id.h>
#include <EngineCore/Runtime/crash_dump.h>
#include <EngineUtils/ErrorHandling/exceptions.h>

#include <EngineCore/Pipeline/asset_enumerable.h>
#include <EngineCore/Runtime/graphics_layer.h>
#include <EngineCore/Runtime/service_table.h>

#include <SDL3/SDL_gpu.h>

using namespace Engine;
using namespace Engine::Extension::RendererModule;

const char* LoggingChannels[] = {"MaterialLoader"};

Core::Runtime::CallbackResult Assets::LoadMaterial(Core::Pipeline::IAssetEnumerator *inputStreams,
                  Core::Runtime::ServiceTable *services,
                  void *moduleState)
{
    ModuleState* state = static_cast<ModuleState*>(moduleState);
    Core::Logging::Logger logger = services->LoggerService->CreateLogger(LoggingChannels, 1);

    while (inputStreams->MoveNext())
    {
        Material material;

        // read pipeline
        inputStreams->GetCurrent().Storage->read((char*)&material.Pipeline, sizeof(material.Pipeline));
        auto pipelineIndex = state->PipelineIndex.find(material.Pipeline);
        if (pipelineIndex == state->PipelineIndex.end())
        {
            logger.Error("Failed to load material {materialId}, pipeline ({pipelineId}) not found.", {inputStreams->GetCurrent().ID, material.Pipeline});
            continue;
        }
        IndexedPipeline& pipeline = state->Pipelines[pipelineIndex->second];

        // read material data
        ConfiguredUniform nextUniform;

        material.VertexUniformStart = state->ConfiguredUniforms.size();
        size_t vertexUniformCount = 0;
        inputStreams->GetCurrent().Storage->read((char*)&vertexUniformCount, sizeof(vertexUniformCount));
        for (size_t i = 0; i < vertexUniformCount; i++)
        {
            inputStreams->GetCurrent().Storage->read((char*)&nextUniform.Binding, sizeof(nextUniform.Binding));
            inputStreams->GetCurrent().Storage->read((char*)&nextUniform.Data, sizeof(nextUniform.Data));
            state->ConfiguredUniforms.push_back(nextUniform);
        }
        material.VertexUniformEnd = state->ConfiguredUniforms.size();

        material.FragmentUniformStart = state->ConfiguredUniforms.size();
        size_t FragmentUniformCount = 0;
        inputStreams->GetCurrent().Storage->read((char*)&FragmentUniformCount, sizeof(FragmentUniformCount));
        for (size_t i = 0; i < FragmentUniformCount; i++)
        {
            inputStreams->GetCurrent().Storage->read((char*)&nextUniform.Binding, sizeof(nextUniform.Binding));
            inputStreams->GetCurrent().Storage->read((char*)&nextUniform.Data, sizeof(nextUniform.Data));
            state->ConfiguredUniforms.push_back(nextUniform);
        }
        material.FragmentUniformEnd = state->ConfiguredUniforms.size();

        state->MaterialIndex[inputStreams->GetCurrent().ID] = { pipelineIndex->second, pipeline.Materials.size() };
        pipeline.Materials.push_back({material});
    }

    return Core::Runtime::CallbackSuccess();
}

Core::Runtime::CallbackResult Assets::UnloadMaterial(Core::Pipeline::HashId *ids, size_t count,
                          Core::Runtime::ServiceTable *services, void *moduleState)
{
    // TODO: implement unloading (asset unloading isn't yet performed)
    return Core::Runtime::Crash(__FILE__, __LINE__, "Unloading material not yet implemented!");
}