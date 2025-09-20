#include "RendererModule/renderer_module.h"

#include "EngineCore/Pipeline/asset_definition.h"
#include "EngineCore/Pipeline/module_definition.h"
#include "RendererModule/Assets/material.h"
#include "RendererModule/Assets/fragment_shader.h"
#include "RendererModule/Assets/mesh.h"
#include "RendererModule/Assets/vertex_shader.h"
#include "md5.h"

using namespace Engine;
using namespace Engine::Extension::RendererModule;

void* InitRendererModule(Core::Runtime::ServiceTable* services)
{
    return new ModuleState();
}

void DisposeRendererModule(Core::Runtime::ServiceTable *services, void *moduleState)
{
    delete static_cast<ModuleState*>(moduleState);
}

Engine::Core::Pipeline::ModuleDefinition Engine::Extension::RendererModule::GetModuleDefinition()
{
    static const Core::Pipeline::AssetDefinition Assets[]
    {
        {
            md5::compute("VertexShader"),
            Assets::LoadVertexShader,
            Assets::UnloadVertexShader
        },
        {
            md5::compute("FragmentShader"),
            Assets::LoadFragmentShader,
            Assets::UnloadFragmentShader
        },
        {
            md5::compute("Material"),
            Assets::LoadMaterial,
            Assets::UnloadMaterial
        },
        {
            md5::compute("Mesh"),
            Assets::LoadMesh,
            Assets::UnloadMesh,
        }
    };

    return Core::Pipeline::ModuleDefinition 
    {
        md5::compute("RendererModule"),
        InitRendererModule,
        DisposeRendererModule,
        Assets,
        sizeof(Assets) / sizeof(Core::Pipeline::AssetDefinition)
    };
}


/*

bool Engine::Core::Runtime::RendererService::QueueRender(RendererMesh *mesh, RendererMaterial *material,
                                                         const glm::mat4 &mvp)
{
    // create command buffer
    SDL_GPUCommandBuffer *cmdbuf = SDL_AcquireGPUCommandBuffer(m_Platform->m_GpuDevice);
    if (cmdbuf == NULL)
    {
        m_Logger->Error(s_ChannelName, "AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        return false;
    }

    // get a target texture
    SDL_GPUTexture *swapchainTexture = nullptr;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdbuf, m_Platform->m_Window, &swapchainTexture, nullptr, nullptr))
    {
        m_Logger->Error(s_ChannelName, "WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
        return false;
    }

    if (swapchainTexture == nullptr)
    {
        SDL_SubmitGPUCommandBuffer(cmdbuf);
        return true;
    }

    // bind pipeline
    SDL_BindGPUGraphicsPipeline(renderPass, material->m_ProgramID);

    // bind mesh (VB and IB)
    SDL_GPUBufferBinding vboBinding{mesh->m_VertexBuffer, 0};
    SDL_BindGPUVertexBuffers(renderPass, 0, &vboBinding, 1);
    SDL_GPUBufferBinding iboBinding{mesh->m_IndexBuffer, 0};
    SDL_BindGPUIndexBuffer(renderPass, &iboBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

    // set the mvp
    SDL_PushGPUVertexUniformData(cmdbuf, 0, &mvp, sizeof(mvp));

    // draw call
    SDL_DrawGPUIndexedPrimitives(renderPass, mesh->m_IndexCount, 1, 0, 0, 0);
    SDL_EndGPURenderPass(renderPass);
    SDL_SubmitGPUCommandBuffer(cmdbuf);

    return true;
}


*/