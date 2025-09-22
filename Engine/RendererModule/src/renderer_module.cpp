#include "RendererModule/renderer_module.h"

#include "RendererModule/Assets/material.h"
#include "RendererModule/Assets/fragment_shader.h"
#include "RendererModule/Assets/mesh.h"
#include "RendererModule/Assets/vertex_shader.h"
#include "RendererModule/Components/mesh_renderer.h"

#include <EngineCore/Pipeline/asset_definition.h>
#include <EngineCore/Pipeline/module_definition.h>
#include <EngineCore/Runtime/service_table.h>
#include <EngineCore/Runtime/graphics_layer.h>
#include <EngineCore/Pipeline/component_definition.h>
#include <EngineCore/Pipeline/engine_callback.h>

#include <SDL3/SDL_gpu.h>
#include <md5.h>
#include <glm/mat4x4.hpp>

using namespace Engine;
using namespace Engine::Extension::RendererModule;

static void* InitRendererModule(Core::Runtime::ServiceTable* services)
{
    return new ModuleState();
}

static void DisposeRendererModule(Core::Runtime::ServiceTable *services, void *moduleState)
{
    delete static_cast<ModuleState*>(moduleState);
}

static void RenderUpdate(Core::Runtime::ServiceTable* services, void* moduleState) 
{
    ModuleState* state = static_cast<ModuleState*>(moduleState);
    SDL_GPUDevice* device = services->GraphicsLayer->GetDevice();
    SDL_GPURenderPass* pass = services->GraphicsLayer->AddRenderPass();

    for (const Components::MeshRenderer& renderTarget : state->MeshRendererComponents)
    {
        auto foundMaterial = state->Materials.find(renderTarget.Material);
        // TODO: default material
        if (foundMaterial == state->Materials.end())
            continue;

        auto foundMesh = state->Meshes.find(renderTarget.Mesh);
        if (foundMesh == state->Meshes.end())
            continue;

        // bind pipeline
        SDL_BindGPUGraphicsPipeline(pass, foundMaterial->second);

        // TODO: load mvp from somewhere
        glm::mat4 mvp;
        SDL_PushGPUVertexUniformData(services->GraphicsLayer->GetCurrentCommandBuffer(), 0, &mvp, sizeof(mvp));

        // bind mesh (VB and IB)
        SDL_GPUBufferBinding vboBinding{foundMesh->second.VertexBuffer, 0};
        SDL_BindGPUVertexBuffers(pass, 0, &vboBinding, 1);
        SDL_GPUBufferBinding iboBinding{foundMesh->second.IndexBuffer, 0};
        SDL_BindGPUIndexBuffer(pass, &iboBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

        // draw
        SDL_DrawGPUIndexedPrimitives(pass, foundMesh->second.IndexCount, 1, 0, 0, 0);
    }

    services->GraphicsLayer->CommitRenderPass(pass);
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

    static const Core::Pipeline::EngineCallback Callbacks[]
    {
        {
            Core::Pipeline::EngineCallbackStage::Render,
            RenderUpdate
        }
    };

    static const Core::Pipeline::ComponentDefinition Components[]
    {
        {
            md5::compute("MeshRenderer"),
            Components::CompileMeshRenderer,
            Components::LoadMeshRenderer
        }
    };

    return Core::Pipeline::ModuleDefinition 
    {
        md5::compute("RendererModule"),
        InitRendererModule,
        DisposeRendererModule,
        Assets,
        sizeof(Assets) / sizeof(Core::Pipeline::AssetDefinition),
        Callbacks,
        sizeof(Callbacks) / sizeof(Core::Pipeline::EngineCallback),
        Components,
        sizeof(Components) / sizeof(Core::Pipeline::ComponentDefinition)
    };
}