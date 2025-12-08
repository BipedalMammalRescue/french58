#include "OrcaRendererModule/Runtime/renderer.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineCore/Runtime/graphics_layer.h"
#include "EngineCore/Runtime/heap_allocator.h"
#include "OrcaRendererModule/Assets/mesh.h"
#include "OrcaRendererModule/Assets/shader.h"
#include "OrcaRendererModule/Runtime/render_context.h"
#include "OrcaRendererModule/Runtime/renderer_resource.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_stdinc.h"

using namespace Engine::Extension::OrcaRendererModule::Runtime;
using namespace Engine::Extension::OrcaRendererModule;

void Renderer::FreeShader(Assets::Shader *shader)
{
    for (uint32_t effectId = 0; effectId < shader->EffectCount; effectId++)
    {
        SDL_ReleaseGPUGraphicsPipeline(m_Services->GraphicsLayer->GetDevice(),
                                       shader->GetShaderEffects()[effectId].Pipeline);
    }
    m_Services->HeapAllocator->Deallocate(shader);
}

// renderer needs to keep its own list of references
bool Renderer::BindShaderResourcesFromSource(Assets::ShaderEffect *shader, size_t resourceCount,
                                             NamedRendererResource *resources,
                                             Assets::ResourceProviderType type,
                                             SDL_GPUCommandBuffer *cmdBuffer,
                                             SDL_GPURenderPass *pass,
                                             Engine::Core::Logging::Logger *logger)
{
    size_t cursor = 0;
    for (size_t i = 0; i < shader->ResourceCount; i++)
    {
        Assets::ShaderResourceBinding *binding = &shader->Resources[i];
        if (binding->Source.ProviderType != type)
            continue;

        while (cursor < resourceCount && resources[cursor].InterfaceName < binding->Source.Name)
        {
            cursor++;
        }

        if (cursor >= resourceCount || resources[cursor].InterfaceName != binding->Source.Name)
        {
            logger->Error("Shader required resource {} not provided.", binding->Source.Name);
            return false;
        }

        const Runtime::RendererResource *resource = GetResource(resources[cursor].ResourceId);

        switch (resource->Type)
        {
        case ResourceType::Texture: {
            switch (binding->Location.Stage)
            {
            case Assets::ShaderStage::Vertex:
                SDL_BindGPUFragmentStorageTextures(pass, binding->Location.Slot, &resource->Texture,
                                                   1);
                break;
            case Assets::ShaderStage::Fragment:
                SDL_BindGPUFragmentStorageTextures(pass, binding->Location.Slot, &resource->Texture,
                                                   1);
                break;
            }
        }
        break;
        case ResourceType::StorageBuffer: {
            switch (binding->Location.Stage)
            {
            case Assets::ShaderStage::Vertex: {
            }
                SDL_BindGPUVertexStorageBuffers(pass, binding->Location.Slot,
                                                &resource->StorageBuffer, 1);
                break;
            case Assets::ShaderStage::Fragment:
                SDL_BindGPUFragmentStorageBuffers(pass, binding->Location.Slot,
                                                  &resource->StorageBuffer, 1);
                break;
            }
        }
        break;
        case ResourceType::UniformBuffer: {
            switch (binding->Location.Stage)
            {
            case Assets::ShaderStage::Vertex:
                SDL_PushGPUVertexUniformData(cmdBuffer, binding->Location.Slot,
                                             resource->Uniform.Data, resource->Uniform.Length);
                break;
            case Assets::ShaderStage::Fragment:
                SDL_PushGPUFragmentUniformData(cmdBuffer, binding->Location.Slot,
                                               resource->Uniform.Data, resource->Uniform.Length);
                break;
            }
        }
        break;
        case ResourceType::Invalid:
            m_Logger.Error("Resource required by shader is invalid on renderer.");
            return false;
        }
    }

    return true;
}

Engine::Core::Runtime::CallbackResult Renderer::Render(SDL_GPUCommandBuffer *cmdBuffer,
                                                       RenderCommand *commands, size_t commandCount)
{
    for (size_t commandIndex = 0; commandIndex < commandCount; commandIndex++)
    {
        switch (commands[commandIndex].Type)
        {
        case RenderCommandType::BeginRenderPass: {
            if (m_CurrentGpuPass != nullptr)
            {
                SDL_EndGPURenderPass(m_CurrentGpuPass);
                m_CurrentGpuPass = nullptr;
            }

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
                m_Logger.Error("Failed to bind render pass, details: {}", SDL_GetError());
                continue;
            }

            m_CurrentGraphPass = pass;
        }
        break;
        case RenderCommandType::BindShader: {
            if (m_CurrentGpuPass == nullptr || m_CurrentGraphPass == nullptr)
            {
                m_Logger.Warning("Shader binding skipped due to invalid render pass binding.");
                continue;
            }

            m_CurrentShader = nullptr;
            Assets::ShaderEffect *shader = commands[commandIndex].BindShader.ShaderEffect;
            SDL_BindGPUGraphicsPipeline(m_CurrentGpuPass, shader->Pipeline);

            // bind the data provided by the graph
            if (!BindShaderResourcesFromSource(shader, m_CurrentGraphPass->InputResourceCount,
                                               m_CurrentGraphPass->InputResources,
                                               Assets::ResourceProviderType::RenderGraph, cmdBuffer,
                                               m_CurrentGpuPass, &m_Logger))
            {
                m_Logger.Error(
                    "Shader binding skipped due to resource binding error, see previous logs for "
                    "details.");
                continue;
            }

            m_CurrentShader = shader;
        }
        break;
        case RenderCommandType::BindMaterial: {
            if (m_CurrentShader == nullptr)
            {
                m_Logger.Warning("Material binding skipped due to invalid shader binding.");
                continue;
            }

            m_CurrentMaterial = nullptr;
            Assets::Material *material = commands[commandIndex].BindMaterial.Material;

            if (!BindShaderResourcesFromSource(
                    m_CurrentShader, material->ResourceCount, material->GetResources(),
                    Assets::ResourceProviderType::Material, cmdBuffer, m_CurrentGpuPass, &m_Logger))
            {
                m_Logger.Error(
                    "Material not bound due to logical error, see previous logs for detail.");
                continue;
            }

            m_CurrentMaterial = material;
        }
        break;
        case RenderCommandType::Draw: {
            if (m_CurrentShader == nullptr || m_CurrentGpuPass == nullptr ||
                m_CurrentMaterial == nullptr)
            {
                m_Logger.Warning(
                    "Draw skipped due to invalid shader, render pass or material binding.");
                continue;
            }

            // bind geometry
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

                // bind the data provided by the graph
                if (!BindShaderResourcesFromSource(
                        m_CurrentShader, commands[commandIndex].Draw.ResourceCount,
                        commands[commandIndex].Draw.Resources, Assets::ResourceProviderType::Object,
                        cmdBuffer, m_CurrentGpuPass, &m_Logger))
                {
                    m_Logger.Error("Draw call skipped due to resource binding error, see "
                                   "previous logs for details.");
                    continue;
                }

                // draw call!
                SDL_DrawGPUIndexedPrimitives(m_CurrentGpuPass, mesh->IndexCount, 1, 0, 0, 0);
            }
        }
        break;
        }
    }

    return Core::Runtime::CallbackSuccess();
}