#include "RendererModule/Assets/material.h"
#include "EngineCore/Logging/logger.h"
#include "RendererModule/renderer_module.h"
#include "RendererModule/Assets/material.h"

#include <EngineCore/Pipeline/hash_id.h>
#include <EngineCore/Runtime/crash_dump.h>
#include <EngineUtils/ErrorHandling/exceptions.h>

#include <EngineCore/Pipeline/asset_enumerable.h>
#include <EngineCore/Logging/logger_service.h>
#include <EngineCore/Runtime/graphics_layer.h>
#include <EngineCore/Runtime/service_table.h>

#include <SDL3/SDL_gpu.h>

using namespace Engine;
using namespace Engine::Extension::RendererModule;

Core::Runtime::CallbackResult Assets::LoadMaterial(Core::Pipeline::IAssetEnumerator *inputStreams,
                  Core::Runtime::ServiceTable *services,
                  void *moduleState)
{
    ModuleState* state = static_cast<ModuleState*>(moduleState);
    Core::Logging::Logger logger = services->LoggerService->CreateLogger("MaterialLoader");

    while (inputStreams->MoveNext())
    {
        Material material;

        // read prototype
        inputStreams->GetCurrent().Storage->read((char*)&material.PrototypeId, sizeof(material.PrototypeId));

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

        state->Materials[inputStreams->GetCurrent().ID] = material;
    }

    return Core::Runtime::CallbackSuccess();
}

Core::Runtime::CallbackResult Assets::UnloadMaterial(Core::Pipeline::HashId *ids, size_t count,
                          Core::Runtime::ServiceTable *services, void *moduleState)
{
    // TODO: implement unloading (asset unloading isn't yet performed)
    return Core::Runtime::Crash(__FILE__, __LINE__, "Unloading material not yet implemented!");
}