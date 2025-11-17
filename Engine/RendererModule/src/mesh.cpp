#include "RendererModule/Assets/mesh.h"
#include "EngineCore/AssetManagement/asset_loading_context.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "EngineUtils/Memory/memstream_lite.h"
#include "RendererModule/renderer_module.h"
#include "RendererModule/Data/vertex.h"

#include "EngineCore/Runtime/service_table.h"
#include "EngineCore/Runtime/graphics_layer.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_gpu.h"

using namespace Engine;
using namespace Engine::Extension::RendererModule;


Core::Runtime::CallbackResult Assets::ContextualizeStaticMesh(Core::Runtime::ServiceTable *services, void *moduleState, Core::AssetManagement::AssetLoadingContext* outContext, size_t contextCount)
{
    RendererModuleState* state = static_cast<RendererModuleState*>(moduleState);
    state->StaticMeshes.reserve(state->StaticMeshes.size() + contextCount);

    // allocate GPU memory
    for (size_t i = 0; i < contextCount; i++)
    {
        if (!state->StaticMeshes.try_emplace(outContext[i].AssetId, StaticMesh{nullptr, 0, nullptr}).second 
            && !outContext[i].ReplaceExisting)
        {
            state->Logger.Information("Static mesh {} is already loaded.", outContext[i].AssetId);
            outContext[i].Buffer.Type = Core::AssetManagement::LoadBufferType::Invalid;
        }
        else 
        {
            SDL_GPUTransferBufferCreateInfo transBufferCreateInfo 
            {
                SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                (uint32_t)outContext[i].SourceSize
            };

            SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(
                services->GraphicsLayer->GetDevice(),
                &transBufferCreateInfo
            );

            if (transferBuffer == nullptr)
            {
                state->Logger.Error("Failed to create transfer buffer for static mesh {}, detail: {}", outContext[i].AssetId, SDL_GetError());
                outContext[i].Buffer.Type = Engine::Core::AssetManagement::LoadBufferType::Invalid;
                continue;
            }

            void *mappedTransferBuffer = SDL_MapGPUTransferBuffer(
                services->GraphicsLayer->GetDevice(), transferBuffer, false);

            if (mappedTransferBuffer == nullptr)
            {
                state->Logger.Error("Failed to map transfer buffer for static mesh {}, detail: {}", outContext[i].AssetId, SDL_GetError());
                SDL_ReleaseGPUTransferBuffer(services->GraphicsLayer->GetDevice(), transferBuffer);
                outContext[i].Buffer.Type = Engine::Core::AssetManagement::LoadBufferType::Invalid;
                continue;
            }

            outContext[i].Buffer.Type = Engine::Core::AssetManagement::LoadBufferType::ModuleBuffer;
            outContext[i].Buffer.Location.ModuleBuffer = mappedTransferBuffer;
            outContext[i].UserData = transferBuffer;
        }
    }

    return Core::Runtime::CallbackSuccess();
}


Core::Runtime::CallbackResult Assets::IndexStaticMesh(Core::Runtime::ServiceTable *services, void *moduleState, Core::AssetManagement::AssetLoadingContext* inContext)
{
    RendererModuleState* state = static_cast<RendererModuleState*>(moduleState);

    void* mappedTransferBuffer = inContext->Buffer.Location.ModuleBuffer;
    SDL_GPUTransferBuffer* transferBuffer = static_cast<SDL_GPUTransferBuffer*>(inContext->UserData);

    // get vertex buffer metadata
    Utils::Memory::MemStreamLite stream = { mappedTransferBuffer, 0 };
    unsigned int vertexCount = stream.Read<unsigned int>();
    unsigned int vertexBufferSize = vertexCount * (uint32_t)sizeof(Data::Vertex);
    unsigned int vertexBufferOffset = stream.GetPosition();

    // skip the vertex buffer region for now
    stream.Seek(vertexBufferOffset + vertexBufferSize);

    // get index buffer metadata
    unsigned int indexCount = stream.Read<unsigned int>();
    unsigned int indexBufferSize = indexCount * (uint32_t)sizeof(int);
    unsigned int indexBufferOffset = stream.GetPosition();

    // close the mapping
    SDL_UnmapGPUTransferBuffer(services->GraphicsLayer->GetDevice(), transferBuffer);
    stream = {nullptr, 0};

    // initiate upload
    SDL_GPUCommandBuffer* uploadCmdBuffer = SDL_AcquireGPUCommandBuffer(services->GraphicsLayer->GetDevice());
    if (uploadCmdBuffer == nullptr)
    {
        state->Logger.Error("Failed to create command buffer for static mesh {}, detail: {}", inContext->AssetId, SDL_GetError());
        return Core::Runtime::CallbackSuccess();
    }
    
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuffer);
    if (copyPass == nullptr)
    {
        state->Logger.Error("Failed to create copy pass for static mesh {}, detail: {}", inContext->AssetId, SDL_GetError());
        return Core::Runtime::CallbackSuccess();
    }

    // upload to vertex buffer
    SDL_GPUBufferCreateInfo vertBufferCreateInfo 
    {
        SDL_GPU_BUFFERUSAGE_VERTEX,
        vertexBufferSize
    };
    SDL_GPUBuffer* vertexBuffer = SDL_CreateGPUBuffer(
        services->GraphicsLayer->GetDevice(),
        &vertBufferCreateInfo
    );

    if (vertexBuffer == nullptr)
    {
        state->Logger.Error("Failed to create vertex buffer object for static mesh {}, detail: {}", inContext->AssetId, SDL_GetError());
    }
    else
    {
        SDL_GPUBufferRegion vertexBufferRegion
        {
            vertexBuffer,
            0,
            vertexBufferSize
        };

        SDL_GPUTransferBufferLocation transBufLocation 
        {
            transferBuffer,
            vertexBufferOffset
        };

        SDL_UploadToGPUBuffer(
            copyPass,
            &transBufLocation,
            &vertexBufferRegion,
            false
        );
    }

    // upload to index buffer
    SDL_GPUBufferCreateInfo indexBufferCreateInfo 
    {
        SDL_GPU_BUFFERUSAGE_INDEX,
        indexBufferSize
    };
    SDL_GPUBuffer* indexBuffer = SDL_CreateGPUBuffer(
        services->GraphicsLayer->GetDevice(),
        &indexBufferCreateInfo
    );

    if (indexBuffer == nullptr)
    {
        state->Logger.Error("Failed to create index buffer object for static mesh {}, detail: {}", inContext->AssetId, SDL_GetError());
    }
    else
    {
        SDL_GPUBufferRegion indexBufferRegion
        {
            indexBuffer,
            0,
            indexBufferSize
        };

        SDL_GPUTransferBufferLocation transBufLocation 
        {
            transferBuffer,
            indexBufferOffset
        };

        SDL_UploadToGPUBuffer(
            copyPass,
            &transBufLocation,
            &indexBufferRegion,
            false
        );
    }

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(uploadCmdBuffer);
    SDL_ReleaseGPUTransferBuffer(services->GraphicsLayer->GetDevice(), transferBuffer);

    Assets::StaticMesh mesh { indexBuffer, indexCount, vertexBuffer };

    auto existingMesh = state->StaticMeshes.try_emplace(inContext->AssetId, mesh);

    // clean up the old value if needed
    if (!existingMesh.second)
    {
        if (existingMesh.first->second.IndexBuffer != nullptr)
        {
            SDL_ReleaseGPUBuffer(services->GraphicsLayer->GetDevice(), existingMesh.first->second.IndexBuffer);
        }
        if (existingMesh.first->second.VertexBuffer != nullptr)
        {
            SDL_ReleaseGPUBuffer(services->GraphicsLayer->GetDevice(), existingMesh.first->second.VertexBuffer);
        }

        existingMesh.first->second = mesh;
    }

    return Core::Runtime::CallbackSuccess();
}