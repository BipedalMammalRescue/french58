#pragma once

#include "EngineCore/Logging/logger.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/service_table.h"
#include "OrcaRendererModule/Assets/material.h"
#include "OrcaRendererModule/Assets/render_graph.h"
#include "OrcaRendererModule/Assets/shader.h"

#include "OrcaRendererModule/Runtime/renderer_resource.h"
#include "SDL3/SDL_gpu.h"

namespace Engine::Extension::OrcaRendererModule::Runtime {

struct RenderCommand;

// Where all the actual rendering logic goes. One stop shop for converting rendering commands into
// rendering calls. Note: most of these logic should be executed on the rendering thread eventually.
// Renderer also doubles as resource management tool.
class Renderer
{
private:
    Core::Logging::Logger m_Logger;
    Core::Runtime::ServiceTable *m_Services;

    // pre-built samplers
    SDL_GPUSampler *m_GpuSamplers[(uint32_t)SamplerType::__Count];

    // resource management
    std::vector<RendererResource> m_Resources;

    // strictly dynamic state
    SDL_GPURenderPass *m_CurrentGpuPass;
    Assets::RenderPass *m_CurrentGraphPass;
    Assets::ShaderEffect *m_CurrentShader;
    Assets::Material *m_CurrentMaterial;

    // only callable on the renderer thread
    bool BindShaderResourcesFromSource(Assets::ShaderEffect *shader, size_t resourceCount,
                                       NamedRendererResource *resources,
                                       Assets::ResourceProviderType type,
                                       SDL_GPUCommandBuffer *cmdBuffer, SDL_GPURenderPass *pass,
                                       Engine::Core::Logging::Logger *logger);

    // only callable on the renderere thread, data can become invalid across frames
    inline const RendererResource *GetResource(uint32_t id) const
    {
        if (id <= m_Resources.size())
            return &m_Resources[id];
        return nullptr;
    }

public:
    Core::Runtime::CallbackResult Render(SDL_GPUCommandBuffer *cmdBuffer, RenderCommand *commands,
                                         size_t commandCount);

    // reserves a new resource ID from the renderer; this resource ID can be used on any type of
    // resource; only safe to call on the simulation thread
    uint32_t CreateResource(const RendererResource &data);

    // assign a resource detail on a previously allocated ID
    // DOES NOT free the underlying resource data
    void ReAssignResource(uint32_t id, const RendererResource &data);

    // shaders are special GPU objects that aren't represented by renderer resource
    void FreeShader(Assets::Shader *shader);
};

} // namespace Engine::Extension::OrcaRendererModule::Runtime