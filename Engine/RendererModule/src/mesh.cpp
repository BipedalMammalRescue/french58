#include "RendererModule/Assets/mesh.h"
#include "RendererModule/renderer_module.h"
#include "RendererModule/Data/vertex.h"

#include "EngineCore/Pipeline/asset_enumerable.h"
#include "EngineCore/Runtime/service_table.h"
#include "EngineCore/Runtime/graphics_layer.h"
#include "SDL3/SDL_gpu.h"
#include <memory>

using namespace Engine;
using namespace Engine::Extension::RendererModule;

void Assets::LoadMesh(Core::Pipeline::IAssetEnumerator *inputStreams,
                        Core::Runtime::ServiceTable *services,
                        void *moduleState)
{
    ModuleState* state = static_cast<ModuleState*>(moduleState);
    state->Meshes.reserve(state->Meshes.size() + inputStreams->Count());
    
    // TODO: need to batch the upload to gpu so the whole process takes one transfer; 
    // ie. read all files in and shove them into one command buffer

    while (inputStreams->MoveNext()) 
    {
        Core::Pipeline::RawAsset asset = inputStreams->GetCurrent();
        std::istream* input = asset.Storage;

        // read the file
        unsigned int vertexCount = 0;
        unsigned int indexCount = 0;

        // fill vertices
        input->read((char*)&vertexCount, sizeof(vertexCount));
        unsigned int vertexBufferSize = vertexCount * (uint32_t)sizeof(Data::Vertex);
        std::unique_ptr<Data::Vertex[]> vertices = std::make_unique<Data::Vertex[]>(vertexCount);
        input->read((char*)vertices.get(), vertexBufferSize);

        // fill indices        
        input->read((char*)&indexCount, sizeof(indexCount));
        unsigned int indexBufferSize = indexCount * (uint32_t)sizeof(int);
        std::unique_ptr<unsigned int[]> indices = std::make_unique<unsigned int[]>(indexCount);
        input->read((char*)indices.get(), indexBufferSize);

        // create buffers for the upload operation
        SDL_GPUBufferCreateInfo vertBufferCreateInfo 
        {
            SDL_GPU_BUFFERUSAGE_VERTEX,
            vertexBufferSize
        };

        SDL_GPUBuffer* vertexBuffer = SDL_CreateGPUBuffer(
            services->GraphicsLayer->GetDevice(),
            &vertBufferCreateInfo
        );

        SDL_GPUBufferCreateInfo indexBufferCreateInfo
        {
            SDL_GPU_BUFFERUSAGE_INDEX,
            indexCount
        };

        SDL_GPUBuffer* indexBuffer = SDL_CreateGPUBuffer(
            services->GraphicsLayer->GetDevice(),
            &indexBufferCreateInfo
        );

        SDL_GPUTransferBufferCreateInfo transBufferCreateInfo 
        {
            SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            vertBufferCreateInfo.size + indexBufferCreateInfo.size
        };

        SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(
            services->GraphicsLayer->GetDevice(),
            &transBufferCreateInfo
        );

        // upload the data
        void* mapping = SDL_MapGPUTransferBuffer(
            services->GraphicsLayer->GetDevice(),
            transferBuffer,
            false
        );
        memcpy(mapping, vertices.get(), vertexBufferSize);
        memcpy((char*)mapping + vertexBufferSize, indices.get(), indexBufferSize);
        SDL_UnmapGPUTransferBuffer(services->GraphicsLayer->GetDevice(), transferBuffer);

        SDL_GPUCommandBuffer* uploadCmdBuffer = SDL_AcquireGPUCommandBuffer(services->GraphicsLayer->GetDevice());
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuffer);
        
        SDL_GPUTransferBufferLocation transBufLocation 
        {
            transferBuffer,
            0
        };

        // upload vertex buffer
        SDL_GPUBufferRegion vertexBufferRegion
        {
            vertexBuffer,
            0,
            vertexBufferSize
        };

        SDL_UploadToGPUBuffer(
            copyPass,
            &transBufLocation,
            &vertexBufferRegion,
            false
        );

        transBufLocation.offset += vertexBufferRegion.size;

        // upload index buffer
        SDL_GPUBufferRegion indexBufferRegion 
        {
            indexBuffer,
            0,
            indexBufferSize
        };

        SDL_UploadToGPUBuffer(
            copyPass,
            &transBufLocation,
            &indexBufferRegion,
            false
        );

        // clean up
        SDL_EndGPUCopyPass(copyPass);
        SDL_SubmitGPUCommandBuffer(uploadCmdBuffer);
        SDL_ReleaseGPUTransferBuffer(services->GraphicsLayer->GetDevice(), transferBuffer);

        // add assets
        Assets::GpuMesh mesh { indexBuffer, indexCount, vertexBuffer };
        state->Meshes[asset.ID] = mesh;
    }
}

void Assets::UnloadMesh(Core::Pipeline::HashId *ids, size_t count,
                          Core::Runtime::ServiceTable *services, void *moduleState)
{
    ModuleState* state = static_cast<ModuleState*>(moduleState);
    
    for (size_t i = 0; i < count; i++)
    {
        auto foundMesh = state->Meshes.find(ids[i]);
        if (foundMesh == state->Meshes.end())
            continue;

        SDL_ReleaseGPUBuffer(services->GraphicsLayer->GetDevice(), foundMesh->second.IndexBuffer);
        SDL_ReleaseGPUBuffer(services->GraphicsLayer->GetDevice(), foundMesh->second.VertexBuffer);

        state->Meshes.erase(foundMesh);
    }
}