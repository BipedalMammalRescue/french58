#pragma once

#include "EngineCore/Pipeline/engine_callback.h"
#include "EngineCore/Runtime/service_table.h"

namespace Engine::Core::Pipeline {

// The core engine library owns the renderer, therefore each module can submit a renderer plugin to
// interact with it.
struct RendererPluginDefinition
{
    void *(*Initialize)(Runtime::ServiceTable *serives, void *moduleState);
    void *(*Dispose)(Runtime::ServiceTable *services, void *moduleState);

    MtRenderUpdateCallback MtWriteRenderStateUpdates;

    RtRenderUpdateCallback RtReadRenderStateUpdates;
    RtRenderSetupCallback RtRenderSetup;
    RtRenderExecuteCallback RtRenderExecute;
};

} // namespace Engine::Core::Pipeline