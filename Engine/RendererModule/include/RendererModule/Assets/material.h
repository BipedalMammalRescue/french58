#pragma once

#include "EngineCore/AssetManagement/asset_loading_context.h"
#include "EngineCore/Pipeline/fwd.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/variant.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/fwd.h"

#include <cstdint>

namespace Engine::Extension::RendererModule::Assets {

Core::Runtime::CallbackResult ContextualizeMaterial(Core::Runtime::ServiceTable *services, void *moduleState, Core::AssetManagement::AssetLoadingContext* outContext, size_t contextCount);
Core::Runtime::CallbackResult IndexMaterial(Core::Runtime::ServiceTable *services, void *moduleState, Core::AssetManagement::AssetLoadingContext* inContext);

struct ConfiguredUniform
{
    uint32_t Binding;
    Core::Pipeline::Variant Data;
};

struct MaterialHeader
{
    Core::Pipeline::HashId PrototypeId;
};

struct Material 
{
    Core::Pipeline::HashId Id;
    MaterialHeader* Header;

    size_t VertUniformOffset;
    size_t VertUniformCount;
    size_t FragUniformOffset;
    size_t FragUniformCount;
};

// sort by prototype id first, then asset id
struct MaterialComparer
{
    static int Compare(const Material* a, const Material* b)
    {
        if (a->Header->PrototypeId < b->Header->PrototypeId)
            return -1;
        if (a->Header->PrototypeId > b->Header->PrototypeId)
            return 1;

        if (a->Id < b->Id)
            return -1;
        if (a->Id > b->Id)
            return 1;
        return 0;
    }
};

}