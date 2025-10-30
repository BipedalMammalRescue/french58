#pragma once

#include "EngineCore/Pipeline/fwd.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/variant.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/fwd.h"

#include <cstdint>

namespace Engine::Extension::RendererModule::Assets {

Core::Runtime::CallbackResult LoadMaterial(Core::Pipeline::IAssetEnumerator *inputStreams,
                        Core::Runtime::ServiceTable *services,
                        void *moduleState);

Core::Runtime::CallbackResult UnloadMaterial(Core::Pipeline::HashId *ids, size_t count,
                          Core::Runtime::ServiceTable *services, void *moduleState);

struct ConfiguredUniform
{
    uint32_t Binding;
    Core::Pipeline::Variant Data;
};

// TODO: I don't have configured texture or storage buffer yet, add them in when implementing texturing

struct Material 
{
    Core::Pipeline::HashId PrototypeId;

    uint32_t VertexUniformStart;
    uint32_t VertexUniformEnd;
    
    uint32_t FragmentUniformStart;
    uint32_t FragmentUniformEnd;
};

}