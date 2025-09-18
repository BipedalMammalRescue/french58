#include "EngineCore/Runtime/renderer_service.h"
#include "EngineCore/Rendering/renderer_data.h"
#include "EngineCore/Runtime/platform_access.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_opengl.h>
#include <glm/mat2x2.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include <stdio.h>

using namespace Engine;
using namespace Engine::Core;

// TODO: how the fuck do I handle errors from SDL?

static const char s_ChannelName[] = "RendererService";

bool Runtime::RendererService::CompileShader(const unsigned char *code, size_t codeLength,
                                                             Rendering::ShaderType type, unsigned int numSamplers,
                                                             unsigned int numUniformBuffers,
                                                             unsigned int numStorageBuffers,
                                                             unsigned int numStorageTextures, RendererShader &outID)
{
    SDL_GPUShaderStage shaderStage;
    switch (type)
    {
    case Rendering::ShaderType::FRAGMENT_SHADER:
        shaderStage = SDL_GPUShaderStage::SDL_GPU_SHADERSTAGE_FRAGMENT;
        break;
    case Rendering::ShaderType::VERTEX_SHADER:
        shaderStage = SDL_GPUShaderStage::SDL_GPU_SHADERSTAGE_VERTEX;
        break;
    }

    SDL_GPUShaderCreateInfo shaderInfo = {codeLength,
                                          code,
                                          "main",
                                          SDL_GPU_SHADERFORMAT_SPIRV,
                                          shaderStage,
                                          numSamplers,
                                          numStorageTextures,
                                          numStorageBuffers,
                                          numUniformBuffers};

    SDL_GPUShader *newShader = SDL_CreateGPUShader(m_Platform->m_GpuDevice, &shaderInfo);
    if (newShader == nullptr)
    {
        return false;
    }

    outID.m_ShaderID = newShader;
    return true;
}

bool Engine::Core::Runtime::RendererService::DeleteShader(RendererShader &shader)
{
    SDL_ReleaseGPUShader(m_Platform->m_GpuDevice, shader.m_ShaderID);
    return false;
}

// it's really "CreatePipeline" amirite
// here comes our big problem: we can't fully decouple vertex layout from the material now, since vertex layout becomes
// part of the pipeline because data is owned by extensions, this method should take a genric representation of the
// layout do I really want to still virtualize layout elements?
bool Engine::Core::Runtime::RendererService::CreateMaterial(const RendererShader &vertexShader,
                                                              const RendererShader &fragmentShader,
                                                              Rendering::VertexAttribute *attributes, unsigned int attributeCount,
                                                              RendererMaterial &outID)
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

bool Engine::Core::Runtime::RendererService::RegisterMesh(const Rendering::VertexCollection &vertices,
                                                            const Rendering::IndexCollection &indices, RendererMesh &outID)
{
    // create buffers for the upload operation
    SDL_GPUBufferCreateInfo vertBufferCreateInfo{SDL_GPU_BUFFERUSAGE_VERTEX, vertices.Size};

    SDL_GPUBuffer *vertexBuffer = SDL_CreateGPUBuffer(m_Platform->m_GpuDevice, &vertBufferCreateInfo);

    SDL_GPUBufferCreateInfo indexBufferCreateInfo{SDL_GPU_BUFFERUSAGE_INDEX, indices.GetSize()};

    SDL_GPUBuffer *indexBuffer = SDL_CreateGPUBuffer(m_Platform->m_GpuDevice, &indexBufferCreateInfo);

    SDL_GPUTransferBufferCreateInfo transBufferCreateInfo{SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                                                          vertBufferCreateInfo.size + indexBufferCreateInfo.size};

    SDL_GPUTransferBuffer *transferBuffer =
        SDL_CreateGPUTransferBuffer(m_Platform->m_GpuDevice, &transBufferCreateInfo);

    // upload the data
    void *mapping = SDL_MapGPUTransferBuffer(m_Platform->m_GpuDevice, transferBuffer, false);
    memcpy(mapping, vertices.Vertices, vertices.Size);
    memcpy((char *)mapping + vertices.Size, indices.Indices, indices.GetSize());
    SDL_UnmapGPUTransferBuffer(m_Platform->m_GpuDevice, transferBuffer);

    SDL_GPUCommandBuffer *uploadCmdBuffer = SDL_AcquireGPUCommandBuffer(m_Platform->m_GpuDevice);
    SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(uploadCmdBuffer);

    SDL_GPUTransferBufferLocation transBufLocation{transferBuffer, 0};

    // upload vertex buffer
    SDL_GPUBufferRegion vertexBufferRegion{vertexBuffer, 0, vertices.Size};

    SDL_UploadToGPUBuffer(copyPass, &transBufLocation, &vertexBufferRegion, false);

    transBufLocation.offset += vertexBufferRegion.size;

    // upload index buffer
    SDL_GPUBufferRegion indexBufferRegion{indexBuffer, 0, indices.GetSize()};

    SDL_UploadToGPUBuffer(copyPass, &transBufLocation, &indexBufferRegion, false);

    // clean up
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(uploadCmdBuffer);
    SDL_ReleaseGPUTransferBuffer(m_Platform->m_GpuDevice, transferBuffer);

    outID.m_IndexBuffer = indexBuffer;
    outID.m_IndexCount = indices.Count;
    outID.m_VertexBuffer = vertexBuffer;
    return true;
}

bool Engine::Core::Runtime::RendererService::DeleteMesh(RendererMesh &inID)
{
    SDL_ReleaseGPUBuffer(m_Platform->m_GpuDevice, inID.m_IndexBuffer);
    SDL_ReleaseGPUBuffer(m_Platform->m_GpuDevice, inID.m_VertexBuffer);
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

    // create render pass
    SDL_GPUColorTargetInfo colorTargetInfo = {0};
    colorTargetInfo.texture = swapchainTexture;
    colorTargetInfo.clear_color = SDL_FColor{0.0f, 0.0f, 0.0f, 1.0f};
    colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass *renderPass = SDL_BeginGPURenderPass(cmdbuf, &colorTargetInfo, 1, NULL);

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
