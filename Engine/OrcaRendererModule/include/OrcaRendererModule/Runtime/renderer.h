#pragma once

#include "EngineCore/Logging/logger.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/service_table.h"
#include "OrcaRendererModule/Assets/material.h"
#include "OrcaRendererModule/Assets/render_graph.h"
#include "OrcaRendererModule/Assets/shader.h"

#include "SDL3/SDL_gpu.h"

namespace Engine::Extension::OrcaRendererModule::Runtime {

struct RenderCommand;

// Where all the actual rendering logic goes. One stop shop for converting rendering commands into
// rendering calls. Note: most of these logic should be executed on the rendering thread eventually.
class Renderer
{
private:
    Core::Logging::Logger m_Logger;

    Core::Runtime::ServiceTable *m_Services;

    SDL_GPURenderPass *m_CurrentGpuPass;
    Assets::RenderPass *m_CurrentGraphPass;
    Assets::ShaderEffect *m_CurrentShader;
    Assets::Material *m_CurrentMaterial;

public:
    Core::Runtime::CallbackResult Render(SDL_GPUCommandBuffer *cmdBuffer, RenderCommand *commands,
                                         size_t commandCount);

    void FreeShader(Assets::Shader *shader);
};

} // namespace Engine::Extension::OrcaRendererModule::Runtime