#include "OrcaRendererModule/Assets/mesh.h"
#include "EngineCore/Runtime/graphics_layer.h"
#include "EngineUtils/Memory/memstream_lite.h"
#include "OrcaRendererModule/orca_renderer_module.h"
#include "SDL3/SDL_gpu.h"

using namespace Engine::Extension::OrcaRendererModule::Assets;
using namespace Engine::Extension::OrcaRendererModule;

struct Vertex
{
    glm::vec3 position{};
    glm::vec3 normal{};
    glm::vec2 uv{};
};

Engine::Core::Runtime::CallbackResult Assets::ContextualizeMesh(
    Engine::Core::Runtime::ServiceTable *services, void *moduleState,
    Engine::Core::AssetManagement::AssetLoadingContext *outContext, size_t contextCount)
{
    ModuleState *state = static_cast<ModuleState *>(moduleState);
    size_t effectiveLoads = 0;

    // allocate GPU memory
    for (size_t i = 0; i < contextCount; i++)
    {
        if (state->FindMesh(outContext[i].AssetId) != nullptr)
        {
            if (!outContext[i].ReplaceExisting)
            {
                state->Logger.Information("Static mesh {} is already loaded.",
                                          outContext[i].AssetId);
                outContext[i].Buffer.Type = Core::AssetManagement::LoadBufferType::Invalid;
                continue;
            }
        }
        else
        {
            effectiveLoads++;
        }

        SDL_GPUTransferBufferCreateInfo transBufferCreateInfo{SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                                                              (uint32_t)outContext[i].SourceSize};

        SDL_GPUTransferBuffer *transferBuffer = SDL_CreateGPUTransferBuffer(
            services->GraphicsLayer->GetDevice(), &transBufferCreateInfo);

        if (transferBuffer == nullptr)
        {
            state->Logger.Error("Failed to create transfer buffer for static mesh {}, detail: {}",
                                outContext[i].AssetId, SDL_GetError());
            outContext[i].Buffer.Type = Engine::Core::AssetManagement::LoadBufferType::Invalid;
            continue;
        }

        void *mappedTransferBuffer =
            SDL_MapGPUTransferBuffer(services->GraphicsLayer->GetDevice(), transferBuffer, false);

        if (mappedTransferBuffer == nullptr)
        {
            state->Logger.Error("Failed to map transfer buffer for static mesh {}, detail: {}",
                                outContext[i].AssetId, SDL_GetError());
            SDL_ReleaseGPUTransferBuffer(services->GraphicsLayer->GetDevice(), transferBuffer);
            outContext[i].Buffer.Type = Engine::Core::AssetManagement::LoadBufferType::Invalid;
            continue;
        }

        outContext[i].Buffer.Type = Engine::Core::AssetManagement::LoadBufferType::ModuleBuffer;
        outContext[i].Buffer.Location.ModuleBuffer = mappedTransferBuffer;
        outContext[i].UserData = transferBuffer;
    }

    return Core::Runtime::CallbackSuccess();
}

// TODO: the GPU copy actions should be scheduled on the rendering thread as part of its resource
// creation stage, so they can all be shoved into one command buffer and maybe during one copy pass
// too
Engine::Core::Runtime::CallbackResult Assets::IndexMesh(
    Engine::Core::Runtime::ServiceTable *services, void *moduleState,
    Engine::Core::AssetManagement::AssetLoadingContext *inContext)
{
    ModuleState *state = static_cast<ModuleState *>(moduleState);

    void *mappedTransferBuffer = inContext->Buffer.Location.ModuleBuffer;
    SDL_GPUTransferBuffer *transferBuffer =
        static_cast<SDL_GPUTransferBuffer *>(inContext->UserData);

    // get vertex buffer metadata
    Utils::Memory::MemStreamLite stream = {mappedTransferBuffer, 0};
    unsigned int vertexCount = stream.Read<unsigned int>();
    unsigned int vertexBufferSize = vertexCount * (uint32_t)sizeof(Vertex);
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
    SDL_GPUCommandBuffer *uploadCmdBuffer =
        SDL_AcquireGPUCommandBuffer(services->GraphicsLayer->GetDevice());
    if (uploadCmdBuffer == nullptr)
    {
        state->Logger.Error("Failed to create command buffer for static mesh {}, detail: {}",
                            inContext->AssetId, SDL_GetError());
        return Core::Runtime::CallbackSuccess();
    }

    SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(uploadCmdBuffer);
    if (copyPass == nullptr)
    {
        state->Logger.Error("Failed to create copy pass for static mesh {}, detail: {}",
                            inContext->AssetId, SDL_GetError());
        return Core::Runtime::CallbackSuccess();
    }

    // upload to vertex buffer
    SDL_GPUBufferCreateInfo vertBufferCreateInfo{SDL_GPU_BUFFERUSAGE_VERTEX, vertexBufferSize};
    SDL_GPUBuffer *vertexBuffer =
        SDL_CreateGPUBuffer(services->GraphicsLayer->GetDevice(), &vertBufferCreateInfo);

    if (vertexBuffer == nullptr)
    {
        state->Logger.Error("Failed to create vertex buffer object for static mesh {}, detail: {}",
                            inContext->AssetId, SDL_GetError());
    }
    else
    {
        SDL_GPUBufferRegion vertexBufferRegion{vertexBuffer, 0, vertexBufferSize};

        SDL_GPUTransferBufferLocation transBufLocation{transferBuffer, vertexBufferOffset};

        SDL_UploadToGPUBuffer(copyPass, &transBufLocation, &vertexBufferRegion, false);
    }

    // upload to index buffer
    SDL_GPUBufferCreateInfo indexBufferCreateInfo{SDL_GPU_BUFFERUSAGE_INDEX, indexBufferSize};
    SDL_GPUBuffer *indexBuffer =
        SDL_CreateGPUBuffer(services->GraphicsLayer->GetDevice(), &indexBufferCreateInfo);

    if (indexBuffer == nullptr)
    {
        state->Logger.Error("Failed to create index buffer object for static mesh {}, detail: {}",
                            inContext->AssetId, SDL_GetError());
    }
    else
    {
        SDL_GPUBufferRegion indexBufferRegion{indexBuffer, 0, indexBufferSize};

        SDL_GPUTransferBufferLocation transBufLocation{transferBuffer, indexBufferOffset};

        SDL_UploadToGPUBuffer(copyPass, &transBufLocation, &indexBufferRegion, false);
    }

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(uploadCmdBuffer);
    SDL_ReleaseGPUTransferBuffer(services->GraphicsLayer->GetDevice(), transferBuffer);

    // record the newly created mesh buffer pair
    Assets::Mesh mesh{
        .IndexCount = indexCount, .VertexBuffer = vertexBuffer, .IndexBuffer = indexBuffer};

    auto oldCopy = state->Meshes.UpdateReference(inContext->AssetId, mesh);
    if (oldCopy.has_value())
    {
        SDL_ReleaseGPUBuffer(services->GraphicsLayer->GetDevice(), oldCopy->IndexBuffer);
        SDL_ReleaseGPUBuffer(services->GraphicsLayer->GetDevice(), oldCopy->VertexBuffer);
    }

    return Core::Runtime::CallbackSuccess();
}
