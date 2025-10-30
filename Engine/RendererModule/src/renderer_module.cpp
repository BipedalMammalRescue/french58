#include "RendererModule/renderer_module.h"

#include "EngineCore/Pipeline/variant.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "RendererModule/Assets/material.h"
#include "RendererModule/Assets/fragment_shader.h"
#include "RendererModule/Assets/mesh.h"
#include "RendererModule/Assets/material.h"
#include "RendererModule/Assets/render_pipeline.h"
#include "RendererModule/Assets/vertex_shader.h"
#include "RendererModule/Components/directional_light.h"
#include "RendererModule/Components/mesh_renderer.h"
#include "RendererModule/configurations.h"
#include "glm/ext/matrix_float4x4.hpp"

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

    for (const auto& pipeline : state->Pipelines)
    {
        SDL_ReleaseGPUGraphicsPipeline(services->GraphicsLayer->GetDevice(), pipeline.Pipeline.GraphicsPipeline);
    }

    for (const auto& mesh : state->Meshes)
    {
        Assets::DisposeMesh(services, mesh.second);
    }

    delete state;
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
    SDL_GPUCommandBuffer* commandBuffer = services->GraphicsLayer->GetCurrentCommandBuffer();

    for (const IndexedPipeline& pipeline : state->Pipelines)
    {
        // set up pipeline
        SDL_BindGPUGraphicsPipeline(pass, pipeline.Pipeline.GraphicsPipeline);

        // static injections
        // TODO: we currently don't inject any static uniforms
        for (uint32_t i = pipeline.Pipeline.StaticVertex.StorageBufferStart; i < pipeline.Pipeline.StaticVertex.StorageBufferEnd; i++)
        {
            Assets::InjectedStorageBuffer buffer = state->InjectedStorageBuffers[i];
        
            switch (buffer.Identifier)
            {
            case (unsigned char)Assets::StaticStorageBufferIdentifier::DirectionalLightBuffer:
                SDL_BindGPUVertexStorageBuffers(pass, buffer.Binding, &state->DirectionalLightBuffer, 1);
                break;
            }
        }
        for (uint32_t i = pipeline.Pipeline.StaticFragment.StorageBufferStart; i < pipeline.Pipeline.StaticFragment.StorageBufferEnd; i++)
        {
            Assets::InjectedStorageBuffer buffer = state->InjectedStorageBuffers[i];
        
            switch (buffer.Identifier)
            {
            case (unsigned char)Assets::StaticStorageBufferIdentifier::DirectionalLightBuffer:
                SDL_BindGPUFragmentStorageBuffers(pass, buffer.Binding, &state->DirectionalLightBuffer, 1);
                break;
            }
        }

        // per-object information
        for (const Components::MeshRenderer& renderTarget : pipeline.Objects)
        {
            // load material uniforms
            // TODO: eventually materials should include more information
            for (uint32_t i = renderTarget.Material.VertexUniformStart; i < renderTarget.Material.VertexUniformEnd; i++)
            {
                Assets::ConfiguredUniform materialUniform = state->ConfiguredUniforms[i];
                SDL_PushGPUVertexUniformData(
                    commandBuffer, 
                    materialUniform.Binding, 
                    &materialUniform.Data.Data, 
                    (uint32_t)Core::Pipeline::GetVariantPayloadSize(materialUniform.Data));
            }
            for (uint32_t i = renderTarget.Material.FragmentUniformStart; i < renderTarget.Material.FragmentUniformEnd; i++)
            {
                Assets::ConfiguredUniform materialUniform = state->ConfiguredUniforms[i];
                SDL_PushGPUFragmentUniformData(
                    commandBuffer, 
                    materialUniform.Binding, 
                    &materialUniform.Data.Data, 
                    (uint32_t)Core::Pipeline::GetVariantPayloadSize(materialUniform.Data));
            }

            // load the MVP, skip if the renderer has no spatial relation
            auto foundModelSpatialRelation = services->ModuleManager->GetRootModule()->SpatialComponents.find(renderTarget.Entity);
            if (foundModelSpatialRelation == services->ModuleManager->GetRootModule()->SpatialComponents.end())
                continue;

            // compute MVP from scene components
            glm::mat4 modelMatrix = foundModelSpatialRelation->second.Transform();

            // bind dynamic injections (only uniforms rn)
            for (uint32_t i = pipeline.Pipeline.DynamicVertex.UniformStart; i < pipeline.Pipeline.DynamicVertex.UniformEnd; i++)
            {
                Assets::InjectedUniform uniform = state->InjectedUniforms[i];
                switch (uniform.Identifier)
                {
                case (unsigned char)Engine::Extension::RendererModule::Assets::DynamicUniformIdentifier::ModelTransform:
                    SDL_PushGPUVertexUniformData(commandBuffer, uniform.Binding, &modelMatrix, sizeof(modelMatrix));
                    break;
                case (unsigned char)Engine::Extension::RendererModule::Assets::DynamicUniformIdentifier::ProjectionTransform:
                    SDL_PushGPUVertexUniformData(commandBuffer, uniform.Binding, &viewMatrix, sizeof(viewMatrix));
                    break;
                case (unsigned char)Engine::Extension::RendererModule::Assets::DynamicUniformIdentifier::ViewTransform:
                    SDL_PushGPUVertexUniformData(commandBuffer, uniform.Binding, &projectMatrix, sizeof(projectMatrix));
                    break;
                }
            }
            for (uint32_t i = pipeline.Pipeline.DynamicFragment.UniformStart; i < pipeline.Pipeline.DynamicFragment.UniformEnd; i++)
            {
                Assets::InjectedUniform uniform = state->InjectedUniforms[i];
                switch (uniform.Identifier)
                {
                case (unsigned char)Engine::Extension::RendererModule::Assets::DynamicUniformIdentifier::ModelTransform:
                    SDL_PushGPUFragmentUniformData(commandBuffer, uniform.Binding, &modelMatrix, sizeof(modelMatrix));
                    break;
                case (unsigned char)Engine::Extension::RendererModule::Assets::DynamicUniformIdentifier::ProjectionTransform:
                    SDL_PushGPUFragmentUniformData(commandBuffer, uniform.Binding, &viewMatrix, sizeof(viewMatrix));
                    break;
                case (unsigned char)Engine::Extension::RendererModule::Assets::DynamicUniformIdentifier::ViewTransform:
                    SDL_PushGPUFragmentUniformData(commandBuffer, uniform.Binding, &projectMatrix, sizeof(projectMatrix));
                    break;
                }
            }

            // bind the mesh
            SDL_GPUBufferBinding vboBinding{renderTarget.Mesh.VertexBuffer, 0};
            SDL_BindGPUVertexBuffers(pass, 0, &vboBinding, 1);
            SDL_GPUBufferBinding iboBinding{renderTarget.Mesh.IndexBuffer, 0};
            SDL_BindGPUIndexBuffer(pass, &iboBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
            
            // draw
            SDL_DrawGPUIndexedPrimitives(pass, renderTarget.Mesh.IndexCount, 1, 0, 0, 0);
        }
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
            md5::compute("RenderPipeline"),
            Assets::LoadRenderPipeline,
            Assets::UnloadRenderPipeline
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