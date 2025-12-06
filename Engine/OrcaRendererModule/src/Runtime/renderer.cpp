#include "OrcaRendererModule/Runtime/renderer.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "OrcaRendererModule/Assets/mesh.h"
#include "OrcaRendererModule/Assets/shader.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_stdinc.h"

using namespace Engine::Extension::OrcaRendererModule::Runtime;

Engine::Core::Runtime::CallbackResult Renderer::Render(SDL_GPUCommandBuffer *cmdBuffer,
                                                       RenderCommand *commands, size_t commandCount)
{
    for (size_t commandIndex = 0; commandIndex < commandCount; commandIndex++)
    {
        switch (commands[commandIndex].Type)
        {
        case RenderCommandType::BeginRenderPass: {
            Assets::RenderPass *pass = commands[commandIndex].BeginShaderPass.Definition;

            // since the render pass is only permitted a very small list
            SDL_GPUColorTargetInfo colorTargets[8] = {0};
            size_t effectiveColorTargetCount =
                SDL_min(pass->ColorTargetCount, SDL_arraysize(colorTargets));

            for (size_t colorTargetIndex = 0; colorTargetIndex < effectiveColorTargetCount;
                 colorTargetIndex++)
            {
                colorTargets[colorTargetIndex] = {};
                colorTargets[colorTargetIndex].texture =
                    pass->ColorTargets[colorTargetIndex].Target;
                colorTargets[colorTargetIndex].load_op =
                    pass->ColorTargets[colorTargetIndex].ShouldClear ? SDL_GPU_LOADOP_CLEAR
                                                                     : SDL_GPU_LOADOP_LOAD;
                colorTargets[colorTargetIndex].store_op = SDL_GPU_STOREOP_STORE;
            }

            SDL_GPUDepthStencilTargetInfo depthStencilTargetInfo = {0};
            depthStencilTargetInfo.texture = pass->DepthStencilTarget.Target;
            depthStencilTargetInfo.load_op =
                pass->DepthStencilTarget.ShouldClear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
            depthStencilTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
            depthStencilTargetInfo.clear_depth = pass->DepthStencilTarget.ClearDepth;

            // TODO: raw API accesses should be tucked away in the engine core.
            m_CurrentGpuPass = SDL_BeginGPURenderPass(
                cmdBuffer, colorTargets, effectiveColorTargetCount, &depthStencilTargetInfo);
            if (m_CurrentGpuPass == nullptr)
            {
                // TODO: need a better crash dump system to automatically tag resource paths
                return Core::Runtime::Crash(__FILE__, __LINE__, "Failed to create render pass.");
            }

            m_CurrentGraphPass = pass;
        }
        break;
        case RenderCommandType::EndRenderPass: {
            SDL_EndGPURenderPass(m_CurrentGpuPass);
            m_CurrentGpuPass = nullptr;
        }
        break;
        case RenderCommandType::BindShader: {
            if (m_CurrentGpuPass == nullptr || m_CurrentGraphPass == nullptr)
                return Core::Runtime::Crash(
                    __FILE__, __LINE__, "Attempting to bind shader before binding a render pass.");

            Assets::ShaderEffect *shader = commands[commandIndex].BindShader.ShaderEffect;
            SDL_BindGPUGraphicsPipeline(m_CurrentGpuPass, shader->Pipeline);
            m_CurrentShader = shader;

            // bind the data provided by the graph
            for (size_t i = 0; i < shader->ResourceCount; i++)
            {
                Assets::ShaderResourceBinding *binding = &shader->Resources[i];
                if (binding->Source.ProviderType != Assets::ResourceProviderType::RenderGraph)
                    continue;

                // TODO: how does the renderer actually handle the resources provided by the graph?
                // I assume when a graph is loaded it needs a companion location in memory to store
                // all of its loaded resource
            }
        }
        break;
        case RenderCommandType::BindMaterial: {
            if (m_CurrentShader == nullptr)
                return Core::Runtime::Crash(__FILE__, __LINE__,
                                            "Attempting to bind material before binding shader.");

            for (size_t i = 0; i < m_CurrentShader->ResourceCount; i++)
            {
                Assets::ShaderResourceBinding *binding = &m_CurrentShader->Resources[i];
                if (binding->Source.ProviderType != Assets::ResourceProviderType::Material)
                    continue;

                // TODO: find the reosurces provided by material (by name)
            }
        }
        break;
        case RenderCommandType::Draw: {
            if (m_CurrentShader == nullptr || m_CurrentGpuPass == nullptr)
                return Core::Runtime::Crash(__FILE__, __LINE__,
                                            "Attempting to draw object before binding shader.");

            // bind geometry
            // TODO: client code should be able to configure vertex binding shape as well
            Assets::Mesh *mesh = commands[commandIndex].Draw.Mesh;
            SDL_GPUBufferBinding vboBinding{mesh->VertexBuffer, 0};
            SDL_BindGPUVertexBuffers(m_CurrentGpuPass, 0, &vboBinding, 1);
            SDL_GPUBufferBinding iboBinding{mesh->IndexBuffer, 0};
            SDL_BindGPUIndexBuffer(m_CurrentGpuPass, &iboBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

            // set shader resources
            for (size_t i = 0; i < m_CurrentShader->ResourceCount; i++)
            {
                Assets::ShaderResourceBinding *binding = &m_CurrentShader->Resources[i];
                if (binding->Source.ProviderType != Assets::ResourceProviderType::Object)
                    continue;

                // TODO:
            }
        }
        break;
        }
    }

    return Core::Runtime::CallbackSuccess();
}