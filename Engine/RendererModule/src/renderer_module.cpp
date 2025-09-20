#include "RendererModule/renderer_module.h"

#include "EngineCore/Pipeline/asset_definition.h"
#include "EngineCore/Pipeline/module_definition.h"
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

// it's really "CreatePipeline" amirite
// here comes our big problem: we can't fully decouple vertex layout from the material now, since vertex layout becomes
// part of the pipeline because data is owned by extensions, this method should take a genric representation of the
// layout do I really want to still virtualize layout elements?
bool Engine::Core::Runtime::RendererService::CreatePipeline(const RendererShader &vertexShader,
                                                            const RendererShader &fragmentShader,
                                                            Rendering::VertexAttribute *attributes,
                                                            unsigned int attributeCount, RendererMaterial &outID)
{
    unsigned int stride = 0;
    std::unique_ptr<SDL_GPUVertexAttribute> vertexAttributes(new SDL_GPUVertexAttribute[attributeCount]);
    for (int i = 0; i < attributeCount; i++)
    {
        SDL_GPUVertexAttribute &current = vertexAttributes.get()[i];
        current.location = i;
        current.buffer_slot = 0;
        current.offset = stride;
        switch (attributes[i].Type)
        {
        case Rendering::GpuDataType::INT32:
            stride += 4 * attributes[i].Count;
            current.format =
                (SDL_GPUVertexElementFormat)((unsigned int)SDL_GPU_VERTEXELEMENTFORMAT_INT + attributes[i].Count - 1);
            break;
        case Rendering::GpuDataType::UINT32:
            stride += 4 * attributes[i].Count;
            current.format =
                (SDL_GPUVertexElementFormat)((unsigned int)SDL_GPU_VERTEXELEMENTFORMAT_UINT + attributes[i].Count - 1);
            break;
        case Rendering::GpuDataType::FLOAT:
            stride += 4 * attributes[i].Count;
            current.format =
                (SDL_GPUVertexElementFormat)((unsigned int)SDL_GPU_VERTEXELEMENTFORMAT_FLOAT + attributes[i].Count - 1);
            break;
        }
    }

    SDL_GPUVertexBufferDescription vertexBufferDescripion{0, stride,
                                                          SDL_GPUVertexInputRate::SDL_GPU_VERTEXINPUTRATE_VERTEX, 0};

    SDL_GPUVertexInputState vertexInputState{// TODO: CONSTRUCT AN ARRAY OF VERTEX BUFFER DESCRIPTIONS
                                             &vertexBufferDescripion, 1, vertexAttributes.get(), attributeCount};

    // TODO: left out a bunch of settings, revisit if it turns out I do need them
    SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo{
        vertexShader.m_ShaderID,
        fragmentShader.m_ShaderID,
        vertexInputState,
        SDL_GPUPrimitiveType::SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
    };

    pipelineCreateInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;

    SDL_GPUColorTargetDescription colorTarget{
        SDL_GetGPUSwapchainTextureFormat(m_Platform->m_GpuDevice, m_Platform->m_Window)};

    pipelineCreateInfo.target_info = {&colorTarget, 1};

    SDL_GPUGraphicsPipeline *newPipeline = SDL_CreateGPUGraphicsPipeline(m_Platform->m_GpuDevice, &pipelineCreateInfo);

    outID.m_ProgramID = newPipeline;
    return newPipeline != nullptr;
}

bool Engine::Core::Runtime::RendererService::DeleteMaterial(RendererMaterial &material)
{
    SDL_ReleaseGPUGraphicsPipeline(m_Platform->m_GpuDevice, material.m_ProgramID);
    return true;
}

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