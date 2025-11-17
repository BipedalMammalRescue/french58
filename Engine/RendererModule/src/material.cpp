#include "RendererModule/Assets/material.h"
#include "EngineCore/AssetManagement/asset_loading_context.h"
#include "EngineCore/Runtime/heap_allocator.h"
#include "EngineUtils/Memory/memstream_lite.h"
#include "RendererModule/common.h"
#include "RendererModule/renderer_module.h"
#include "RendererModule/Assets/material.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/service_table.h"

using namespace Engine;
using namespace Engine::Extension::RendererModule;

Core::Runtime::CallbackResult Assets::ContextualizeMaterial(Core::Runtime::ServiceTable *services, void *moduleState, Core::AssetManagement::AssetLoadingContext* outContext, size_t contextCount)
{
    // calculate the total size needed
    RendererModuleState* state = static_cast<RendererModuleState*>(moduleState);
    for (size_t i = 0; i < contextCount; i++)
    {
        // check if the incoming load is duplicate
        if (!state->LoadedMaterials.TryInsert(outContext[i].AssetId) && !outContext[i].ReplaceExisting)
        {
            state->Logger.Information("Material {} is already loaded.", outContext[i].AssetId);
            outContext[i].Buffer.Type = Core::AssetManagement::LoadBufferType::Invalid;
        }
        else 
        {
            outContext[i].Buffer.Type = Engine::Core::AssetManagement::LoadBufferType::ModuleBuffer;
        }
    }

    // distribute out the pointers
    for (size_t i = 0; i < contextCount; i++)
    {
        if (outContext[i].Buffer.Type == Engine::Core::AssetManagement::LoadBufferType::Invalid)
            continue;
        outContext[i].Buffer.Location.ModuleBuffer = services->HeapAllocator->Allocate(outContext[i].SourceSize);
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
    state->MaterialIndex.Replace(material);
    
    return Core::Runtime::CallbackSuccess();
}