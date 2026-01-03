#pragma once

#include "EngineCore/Rendering/render_thread.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/graphics_layer.h"
#include "EngineCore/Runtime/service_table.h"

namespace Engine::Core::Pipeline {

// The core engine library owns the renderer, therefore each module can submit a renderer plugin to
// interact with it.
struct RendererPlugin
{
    void *(*Initialize)(Runtime::ServiceTable *);
    void *(*Dispose)(Runtime::ServiceTable *);

    // this callback is always called on the main thread
    Runtime::CallbackResult (*WriteRenderStateUpdates)(Runtime::ServiceTable *services,
                                                       void *moduleState,
                                                       Rendering::IRenderStateUpdateWriter *writer);

    // this callback is always called on the render thread
    Runtime::CallbackResult (*ReadRenderStateUpdates)(const Runtime::GraphicsLayer *graphicsLayer,
                                                      void *pluginState,
                                                      Rendering::IRenderStateUpdateReader *reader);
};

} // namespace Engine::Core::Pipeline