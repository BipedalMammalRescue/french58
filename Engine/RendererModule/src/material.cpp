#include "RendererModule/Assets/material.h"
#include "EngineUtils/Memory/memstream_lite.h"
#include "RendererModule/common.h"
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

Core::Runtime::CallbackResult Assets::ContextualizeMaterial(Core::Runtime::ServiceTable *services, void *moduleState, Core::AssetManagement::AssetLoadingContext* outContext, size_t contextCount)
{
    // calculate the total size needed
    RendererModuleState* state = static_cast<RendererModuleState*>(moduleState);
    size_t incomingSize = 0;
    for (size_t i = 0; i < contextCount; i++)
    {
        incomingSize += outContext[i].SourceSize;
    }

    // bulk allocate buffer
    size_t originalSize = state->MaterialStorage.size();
    state->MaterialStorage.resize(originalSize + incomingSize);

    // distribute out the pointers
    for (size_t i = 0; i < contextCount; i++)
    {
        outContext[i].Buffer.Type = Engine::Core::AssetManagement::LoadBufferType::ModuleBuffer;
        outContext[i].Buffer.Location.ModuleBuffer = state->MaterialStorage.data() + originalSize;
        originalSize += outContext[i].SourceSize;
    }

    // allocate space in the index
    state->MaterialIndex.ReserveExtra(contextCount);

    return Engine::Core::Runtime::CallbackSuccess();
}


Core::Runtime::CallbackResult Assets::IndexMaterial(Core::Runtime::ServiceTable *services, void *moduleState, Core::AssetManagement::AssetLoadingContext* inContext)
{
    RendererModuleState* state = static_cast<RendererModuleState*>(moduleState);

    Assets::MaterialHeader *header = static_cast<Assets::MaterialHeader*>(inContext->Buffer.Location.ModuleBuffer);
    Utils::Memory::MemStreamLite stream = { SkipHeader(header), 0 };

    size_t vertUniformCount = stream.Read<size_t>();
    size_t vertUniformOffset = stream.GetPosition();

    stream.Seek(vertUniformOffset + vertUniformCount * sizeof(Assets::ConfiguredUniform));

    size_t fragUniformCount = stream.Read<size_t>();
    size_t fragUniformOffset = stream.GetPosition();

    Assets::Material material = {
        inContext->AssetId,
        header,
        vertUniformOffset,
        vertUniformCount,
        fragUniformOffset,
        fragUniformCount
    };
    state->MaterialIndex.Insert(material);
    
    return Core::Runtime::CallbackSuccess();
}