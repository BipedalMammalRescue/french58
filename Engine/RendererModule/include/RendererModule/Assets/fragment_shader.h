#pragma once

#include "EngineCore/Pipeline/fwd.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Runtime/fwd.h"

#include <cstddef>

namespace Engine::Extension::RendererModule::Assets {

void LoadFragmentShader(Core::Pipeline::AssetEnumerable *inputStreams,
                        Core::Runtime::ServiceTable *services,
                        void *moduleState);
                        
void UnloadFragmentShader(Core::Pipeline::HashId *ids, size_t count,
                          Core::Runtime::ServiceTable *services, void *moduleState);

} // namespace Engine::Extension::RendererModule::Assets