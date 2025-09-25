#pragma once

#include "EngineCore/Pipeline/fwd.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Runtime/fwd.h"

namespace Engine::Extension::RendererModule::Assets {

void LoadVertexShader(Core::Pipeline::IAssetEnumerator *inputStreams,
                        Core::Runtime::ServiceTable *services,
                        void *moduleState);
                        
void UnloadVertexShader(Core::Pipeline::HashId *ids, size_t count,
                          Core::Runtime::ServiceTable *services, void *moduleState);

}