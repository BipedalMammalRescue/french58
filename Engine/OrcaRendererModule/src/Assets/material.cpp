#include "OrcaRendererModule/Assets/material.h"
#include "EngineCore/AssetManagement/asset_loading_context.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/heap_allocator.h"
#include "OrcaRendererModule/Runtime/renderer_resource.h"
#include "OrcaRendererModule/orca_renderer_module.h"
#include "SDL3/SDL_stdinc.h"

using namespace Engine::Extension::OrcaRendererModule::Assets;
using namespace Engine::Extension::OrcaRendererModule;

// The material file would reference the actual file that provides such texture; we have the room to
// do a little bit polymorphism here.
struct SerializedMaterialTexture
{
    Engine::Core::Pipeline::HashId Name;
    Engine::Core::Pipeline::HashId TexturePath;
};

struct SerializedMaterial
{
    uint32_t TextureCount;

    const SerializedMaterialTexture *GetTextures() const
    {
        return (SerializedMaterialTexture *)(this + 1);
    }
};

Engine::Core::Runtime::CallbackResult Assets::ContextualizeMaterial(
    Engine::Core::Runtime::ServiceTable *services, void *moduleState,
    Engine::Core::AssetManagement::AssetLoadingContext *outContext, size_t contextCount)
{
    ModuleState *state = static_cast<ModuleState *>(moduleState);

    size_t effectiveLoadSize = 0;

    for (size_t i = 0; i < contextCount; i++)
    {
        // check if the material is a duplicate
        if (!outContext[i].ReplaceExisting && state->FindMaterial(outContext[i].AssetId) != nullptr)
        {
            state->Logger.Information("Material {} already loaded.", outContext[i].AssetId);
            outContext[i].Buffer.Type = Core::AssetManagement::LoadBufferType::Invalid;
            continue;
        }

        // get a transient buffer to old that data, since we would immediately translate that hash
        // id to an immutable reference
        outContext[i].Buffer.Type = Core::AssetManagement::LoadBufferType::TransientBuffer;
        outContext[i].Buffer.Location.TransientBufferSize = outContext->SourceSize;
        effectiveLoadSize++;
    }

    // pre-allocate the reference nodes
    state->Materials.ReserveExtra(effectiveLoadSize);
    return Core::Runtime::CallbackSuccess();
}

Engine::Core::Runtime::CallbackResult Assets::IndexFragmentMaterial(
    Engine::Core::Runtime::ServiceTable *services, void *moduleState,
    Engine::Core::AssetManagement::AssetLoadingContext *inContext)
{
    ModuleState *state = static_cast<ModuleState *>(moduleState);

    SerializedMaterial *loadedMaterial = static_cast<SerializedMaterial *>(
        services->TransientAllocator->GetBuffer(inContext->Buffer.Location.TransientBufferId));
    if (loadedMaterial == nullptr)
    {
        state->Logger.Error("Failed to index material {} due to invalid transient buffer.",
                            inContext->AssetId);
        return Core::Runtime::CallbackSuccess();
    }

    // materials are heap objects due to their varying size
    size_t objectSize =
        sizeof(Material) + loadedMaterial->TextureCount * sizeof(Runtime::NamedRendererResource);
    Material *newMaterial = static_cast<Material *>(services->HeapAllocator->Allocate(objectSize));

    // make the new material available
    state->Materials.UpdateReference(inContext->AssetId, newMaterial);

    // build the asset table (for a material currently it's a list of textures, managed by the
    // renderer)
    for (size_t i = 0; i < loadedMaterial->TextureCount; i++)
    {
        const SerializedMaterialTexture *textureRef = &loadedMaterial->GetTextures()[i];
        Core::Containers::Uniform::AnnotatedNode<Core::Pipeline::HashId, unsigned int> *texture =
            state->Textures.GetOrAdd({.Key = textureRef->TexturePath, .Value = SDL_MAX_UINT32});

        // somehow this texture has not been created yet
        if (texture->Value == SDL_MAX_UINT32)
        {
            state->Logger.Error("Asset dependency error: material {} depends on texture {}, but "
                                "the latter has not been loaded at least once.",
                                inContext->AssetId, textureRef->TexturePath);

            uint32_t newTextureId = state->Renderer.CreateResource({
                .Type = Runtime::ResourceType::Invalid,
            });
            texture->Value = newTextureId;
        }

        newMaterial->GetResources()[i] = {.InterfaceName = textureRef->Name,
                                          .ResourceId = texture->Value};
    }

    state->Logger.Information("Indexed material {} with {} texture references.", inContext->AssetId,
                              loadedMaterial->TextureCount);
    return Core::Runtime::CallbackSuccess();
}
