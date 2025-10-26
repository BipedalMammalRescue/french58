#include "RendererModule/renderer_module.h"

#include "EngineCore/Runtime/crash_dump.h"
#include "RendererModule/Assets/material.h"
#include "RendererModule/Assets/fragment_shader.h"
#include "RendererModule/Assets/mesh.h"
#include "RendererModule/Assets/vertex_shader.h"
#include "RendererModule/Components/directional_light.h"
#include "RendererModule/Components/mesh_renderer.h"
#include "RendererModule/configurations.h"
#include "glm/ext/vector_float3.hpp"

#include <EngineCore/Pipeline/asset_definition.h>
#include <EngineCore/Pipeline/module_definition.h>
#include <EngineCore/Runtime/service_table.h>
#include <EngineCore/Runtime/graphics_layer.h>
#include <EngineCore/Pipeline/component_definition.h>
#include <EngineCore/Pipeline/engine_callback.h>
#include <EngineCore/Runtime/module_manager.h>
#include <EngineCore/Ecs/Components/spatial_component.h>
#include <EngineCore/Ecs/Components/camera_component.h>

#include <SDL3/SDL_gpu.h>
#include <md5.h>
#include <glm/mat4x4.hpp>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE 1

using namespace Engine;
using namespace Engine::Extension::RendererModule;

static void* InitRendererModule(Core::Runtime::ServiceTable* services)
{
    SDL_GPUBufferCreateInfo createInfo {
        SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
        8
    };

    SDL_GPUBuffer* emptyBuffer = SDL_CreateGPUBuffer(services->GraphicsLayer->GetDevice(), &createInfo);

    return new ModuleState { services->ModuleManager->GetRootModule(), emptyBuffer };
}

// TODO: need a better way to collect
static void DisposeRendererModule(Core::Runtime::ServiceTable *services, void *moduleState)
{
    ModuleState* state = static_cast<ModuleState*>(moduleState);

    SDL_ReleaseGPUBuffer(services->GraphicsLayer->GetDevice(), state->EmptyStorageBuffer);
    SDL_ReleaseGPUBuffer(services->GraphicsLayer->GetDevice(), state->DirectionalLightBuffer);

    for (const auto& shader : state->FragmentShaders)
    {
        Assets::DisposeFragmentShader(services, shader.second);
    }

    for (const auto& shader : state->VertexShaders)
    {
        SDL_ReleaseGPUShader(services->GraphicsLayer->GetDevice(), shader.second);
    }

    for (const auto& material : state->LegacyMaterials)
    {
        SDL_ReleaseGPUGraphicsPipeline(services->GraphicsLayer->GetDevice(), material.second);
    }

    for (const auto& mesh : state->Meshes)
    {
        Assets::DisposeMesh(services, mesh.second);
    }

    delete static_cast<ModuleState*>(moduleState);
}

static Core::Runtime::CallbackResult RenderUpdate(Core::Runtime::ServiceTable* services, void* moduleState) 
{
    // get the primary engine camera
    int cameraEntity = -1;
    for (const Core::Ecs::Components::Camera& candidate : services->ModuleManager->GetRootModule()->CameraComponents) 
    {
        if (candidate.IsPrimary)
        {
            cameraEntity = candidate.Entity;
            break;
        }
    }

    // abort if primary camera doesn't exist or it doesn't have a valid spatial relation
    if (cameraEntity == -1)
        return Core::Runtime::CallbackSuccess();
    auto foundCameraTransform = services->ModuleManager->GetRootModule()->SpatialComponents.find(cameraEntity);
    if (foundCameraTransform == services->ModuleManager->GetRootModule()->SpatialComponents.end())
        return Core::Runtime::CallbackSuccess();

    // calculate view matrix
    glm::mat4 viewMatrix = glm::inverse(foundCameraTransform->second.Transform());

    // calculate projection matrix
    glm::mat4 projectMatrix =
        glm::perspective(glm::radians<float>(Configuration::FieldOfView),
                         960.0f / 720.0f, 0.1f, 500.0f);

    // pre-calculate the first part of MVP
    glm::mat4 pvMatrix = projectMatrix * viewMatrix;

    // iterate all models
    ModuleState* state = static_cast<ModuleState*>(moduleState);
    SDL_GPUDevice* device = services->GraphicsLayer->GetDevice();
    SDL_GPURenderPass* pass = services->GraphicsLayer->AddRenderPass();

    for (const Components::MeshRenderer& renderTarget : state->MeshRendererComponents)
    {
        // load the MVP, skip if the renderer has no spatial relation
        auto foundModelSpatialRelation = services->ModuleManager->GetRootModule()->SpatialComponents.find(renderTarget.Entity);
        if (foundModelSpatialRelation == services->ModuleManager->GetRootModule()->SpatialComponents.end())
            continue;

        // compute MVP from scene components
        glm::mat4 modelMatrix = foundModelSpatialRelation->second.Transform();

        // find the material
        // TODO: default material when it's not found
        auto foundMaterial = state->LegacyMaterials.find(renderTarget.Material);
        if (foundMaterial == state->LegacyMaterials.end())
            continue;

        // find the mesh, abort if there's no mesh
        auto foundMesh = state->Meshes.find(renderTarget.Mesh);
        if (foundMesh == state->Meshes.end())
            continue;

        // bind the light buffer
        if (state->DirectionalLightBuffer != nullptr)
        {
            SDL_BindGPUFragmentStorageBuffers(pass, 0, &state->DirectionalLightBuffer, 1);
        }
        else 
        {
            SDL_BindGPUFragmentStorageBuffers(pass, 0, &state->EmptyStorageBuffer, 1);
        }

        // bind pipeline
        SDL_BindGPUGraphicsPipeline(pass, foundMaterial->second);

        // insert model matrix
        SDL_PushGPUVertexUniformData(services->GraphicsLayer->GetCurrentCommandBuffer(), 0, &modelMatrix, sizeof(modelMatrix));
        SDL_PushGPUVertexUniformData(services->GraphicsLayer->GetCurrentCommandBuffer(), 1, &viewMatrix, sizeof(viewMatrix));
        SDL_PushGPUVertexUniformData(services->GraphicsLayer->GetCurrentCommandBuffer(), 2, &projectMatrix, sizeof(projectMatrix));

        // SDL_PushGPUVertexUniformData(services->GraphicsLayer->GetCurrentCommandBuffer(), 1, &pvMatrix, sizeof(pvMatrix));
        // SDL_PushGPUVertexUniformData(services->GraphicsLayer->GetCurrentCommandBuffer(), 2, &foundCameraTransform->second.Translation, sizeof(glm::vec3));

        // bind mesh (VB and IB)
        SDL_GPUBufferBinding vboBinding{foundMesh->second.VertexBuffer, 0};
        SDL_BindGPUVertexBuffers(pass, 0, &vboBinding, 1);
        SDL_GPUBufferBinding iboBinding{foundMesh->second.IndexBuffer, 0};
        SDL_BindGPUIndexBuffer(pass, &iboBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

        // draw
        SDL_DrawGPUIndexedPrimitives(pass, foundMesh->second.IndexCount, 1, 0, 0, 0);
    }

    services->GraphicsLayer->CommitRenderPass(pass);
    return Core::Runtime::CallbackSuccess();
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
            md5::compute("SlangVertexShader"),
            Assets::LoadVertexShader,
            Assets::UnloadVertexShader
        },
        {
            md5::compute("SlangFragmentShader"),
            Assets::LoadFragmentShader,
            Assets::UnloadFragmentShader
        },
        {
            md5::compute("Material"),
            Assets::LoadLegacyMaterial,
            Assets::UnloadLegacyMaterial
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
        },
        {
            md5::compute("DirectionalLight"),
            Components::CompileDirectionalLight,
            Components::LoadDirectionalLight
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