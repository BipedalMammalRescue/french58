#include "RendererModule/renderer_module.h"

#include "EngineCore/Logging/logger_service.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Runtime/container_factory_service.h"
#include "RendererModule/Assets/material.h"
#include "RendererModule/Assets/fragment_shader.h"
#include "RendererModule/Assets/mesh.h"
#include "RendererModule/Assets/material.h"
#include "RendererModule/Assets/render_pipeline.h"
#include "RendererModule/Assets/vertex_shader.h"
#include "RendererModule/Components/directional_light.h"
#include "RendererModule/Components/mesh_renderer.h"
#include "RendererModule/configurations.h"
#include "RendererModule/common.h"

#include <EngineCore/Pipeline/variant.h>
#include <EngineCore/Runtime/crash_dump.h>
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
#include <glm/ext/matrix_float4x4.hpp>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE 1

using namespace Engine;
using namespace Engine::Extension::RendererModule;

static SDL_GPUBuffer* CreaetEmptyStorageBuffer(Core::Runtime::GraphicsLayer* gl)
{
    SDL_GPUBufferCreateInfo createInfo {
        SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
        8
    };

    return SDL_CreateGPUBuffer(gl->GetDevice(), &createInfo);
}

RendererModuleState::RendererModuleState(Core::Runtime::ServiceTable* services)
    : RootModule(services->ModuleManager->GetRootModule()),
    EmptyStorageBuffer(CreaetEmptyStorageBuffer(services->GraphicsLayer)),
    Logger(services->LoggerService->CreateLogger("RendererModule")),
    PipelineIndex(services->ContainerFactory->CreateSortedArray<Assets::RenderPipeline, Assets::RenderPipelineComparer>(16)),
    LoadedMaterials(services->ContainerFactory->CreateSortedArray<Core::Pipeline::HashId>(16)),
    MaterialIndex(services->ContainerFactory->CreateSortedArray<Assets::Material, Assets::MaterialComparer>(16)),
    MeshRenderers(services->ContainerFactory->CreateSortedArray<Components::MeshRenderer, Components::MeshRendererComparer>(16)),
    DirectionalLightBuffer(nullptr)
{}

static void* InitRendererModule(Core::Runtime::ServiceTable* services)
{
    return new RendererModuleState(services);
}

static void DisposeRendererModule(Core::Runtime::ServiceTable *services, void *moduleState)
{
    RendererModuleState* state = static_cast<RendererModuleState*>(moduleState);

    SDL_ReleaseGPUBuffer(services->GraphicsLayer->GetDevice(), state->EmptyStorageBuffer);
    SDL_ReleaseGPUBuffer(services->GraphicsLayer->GetDevice(), state->DirectionalLightBuffer);

    for (const auto& mesh : state->StaticMeshes)
    {
        SDL_ReleaseGPUBuffer(services->GraphicsLayer->GetDevice(), mesh.second.IndexBuffer);
        SDL_ReleaseGPUBuffer(services->GraphicsLayer->GetDevice(), mesh.second.VertexBuffer);
    }

    for (size_t i = 0; i < state->PipelineIndex.GetCount(); i++)
    {
        if (state->PipelineIndex.PtrAt(i)->GpuPipeline == nullptr)
            continue;
        SDL_ReleaseGPUGraphicsPipeline(services->GraphicsLayer->GetDevice(), state->PipelineIndex.PtrAt(i)->GpuPipeline);
    }

    for (const auto& shader : state->FragmentShaders)
    {
        if (shader.second == nullptr)
            continue;
        SDL_ReleaseGPUShader(services->GraphicsLayer->GetDevice(), shader.second);
    }

    for (const auto& shader : state->VertexShaders)
    {
        if (shader.second == nullptr)
            continue;
        SDL_ReleaseGPUShader(services->GraphicsLayer->GetDevice(), shader.second);
    }

    // destroy the borrowed containers
    state->MaterialIndex.Destroy();
    state->PipelineIndex.Destroy();
    state->MeshRenderers.Destroy();

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
                         960.0f / 720.0f, 0.1f, 10000.0f);

    // pre-calculate the first part of MVP
    glm::mat4 pvMatrix = projectMatrix * viewMatrix;

    // iterate all models
    RendererModuleState* state = static_cast<RendererModuleState*>(moduleState);
    SDL_GPUDevice* device = services->GraphicsLayer->GetDevice();
    SDL_GPURenderPass* pass = services->GraphicsLayer->AddRenderPass();
    SDL_GPUCommandBuffer* commandBuffer = services->GraphicsLayer->GetCurrentCommandBuffer();

    // cache the mesh renderer position
    size_t meshRendererPos = 0;

    // foreach pipeline
    for (size_t pipelinePosition = 0; pipelinePosition < state->PipelineIndex.GetCount(); pipelinePosition++)
    {
        // adjust the mesh renderer id and pipeline id so they match
        while (meshRendererPos < state->MeshRenderers.GetCount()
            && pipelinePosition < state->PipelineIndex.GetCount()
            && state->MeshRenderers.PtrAt(meshRendererPos)->Pipeline != state->PipelineIndex.PtrAt(pipelinePosition)->Id)
        {
            if (state->MeshRenderers.PtrAt(meshRendererPos)->Pipeline > state->PipelineIndex.PtrAt(pipelinePosition)->Id)
            {
                pipelinePosition ++;
            }
            else if (state->MeshRenderers.PtrAt(meshRendererPos)->Pipeline > state->PipelineIndex.PtrAt(pipelinePosition)->Id)
            {
                meshRendererPos++;
            }
        }

        // if nothing matches after this point, break
        if (pipelinePosition == state->PipelineIndex.GetCount() || meshRendererPos == state->MeshRenderers.GetCount())
            break;

        // dereference the pipeline and mesh renderer
        auto currentPipeline = state->PipelineIndex.PtrAt(pipelinePosition);

        // skip a pipeline if it's not available
        if (currentPipeline->GpuPipeline == nullptr)
            continue;
        SDL_BindGPUGraphicsPipeline(pass, currentPipeline->GpuPipeline);

        // find the first material that matches the prototype id
        size_t materialPos = state->MaterialIndex.GetCount();
        {
            // search for any material with matching prototype (materials are sorted based on prototype first and id second)
            struct MaterialFinder
            {
                static int Compare(const Assets::Material* a, const Assets::Material* b)
                {
                    if (a->Header->PrototypeId < b->Header->PrototypeId)
                        return -1;
                    if (a->Header->PrototypeId > b->Header->PrototypeId)
                        return 1;
                    return 0;
                }
            };
            Assets::MaterialHeader templateMaterialHeader { currentPipeline->Header->PrototypeId };
            Assets::Material templateMaterial { .Header = &templateMaterialHeader };
            materialPos = state->MaterialIndex.CustomSearch<MaterialFinder>(&templateMaterial);

            // no compatible materials, abort
            if (materialPos >= state->MaterialIndex.GetCount())
                continue;

            // loop back material position to find the first material bearing said prototype id
            while (materialPos > 0 && state->MaterialIndex.PtrAt(materialPos - 1)->Header->PrototypeId == currentPipeline->Header->PrototypeId)
            {
                materialPos --;
            }
        }

        // process injections
        auto loadedPipelineData = static_cast<char*>(SkipHeader(currentPipeline->Header));

        // static injections
        // NOTE: we currently don't inject any static uniforms
        Assets::InjectedStorageBuffer* staticVertStorageBuffers = (Assets::InjectedStorageBuffer*)(loadedPipelineData + currentPipeline->StaticVertStorageBuffer.Offset);
        for (size_t i = 0; i < currentPipeline->StaticVertStorageBuffer.Count; i++) 
        {
            switch (staticVertStorageBuffers[i].Identifier) 
            {
            case (unsigned char)Assets::StaticStorageBufferIdentifier::DirectionalLightBuffer:
                SDL_BindGPUVertexStorageBuffers(pass, staticVertStorageBuffers[i].Binding, &state->DirectionalLightBuffer, 1);
                break;
            }
        }

        Assets::InjectedStorageBuffer* staticFragStorageBuffers = (Assets::InjectedStorageBuffer*)(loadedPipelineData + currentPipeline->StaticFragStorageBuffer.Offset);
        for (size_t i = 0; i < currentPipeline->StaticFragStorageBuffer.Count; i++) 
        {
            switch (staticFragStorageBuffers[i].Identifier)
            {
            case (unsigned char)Assets::StaticStorageBufferIdentifier::DirectionalLightBuffer:
                SDL_BindGPUFragmentStorageBuffers(pass, staticFragStorageBuffers[i].Binding, &state->DirectionalLightBuffer, 1);
                break;
            }
        }

        // loop through mesh renderers
        size_t previouslyActiveMaterialPos = state->MaterialIndex.GetCount();
        for (; meshRendererPos < state->MeshRenderers.GetCount() && state->MeshRenderers.PtrAt(meshRendererPos)->Pipeline == currentPipeline->Id; meshRendererPos ++)
        {
            // adjust positions until we have a matching material-object pair
            while (materialPos < state->MaterialIndex.GetCount()
                && meshRendererPos < state->MeshRenderers.GetCount()
                && state->MaterialIndex.PtrAt(materialPos)->Id != state->MeshRenderers.PtrAt(meshRendererPos)->Material)
            {
                if (state->MaterialIndex.PtrAt(materialPos)->Id < state->MeshRenderers.PtrAt(meshRendererPos)->Material)
                {
                    materialPos ++;
                }
                else
                {
                    meshRendererPos ++;
                }
            }

            if (materialPos >= state->MaterialIndex.GetCount() || meshRendererPos >= state->MeshRenderers.GetCount())
                break;

            auto currentMeshRenderer = state->MeshRenderers.PtrAt(meshRendererPos);
            auto currentMaterial = state->MaterialIndex.PtrAt(materialPos);

            // handle missing assets here
            if (currentMeshRenderer->VertexBuffer == nullptr || currentMeshRenderer->IndexBuffer == nullptr)
            {
                // skip this mesh renderer if the target mesh isn't in yet
                auto foundMesh = state->StaticMeshes.find(currentMeshRenderer->Mesh);
                if (foundMesh == state->StaticMeshes.end() || foundMesh->second.IndexBuffer == nullptr || foundMesh->second.VertexBuffer == nullptr)
                    continue;

                currentMeshRenderer->IndexBuffer = foundMesh->second.IndexBuffer;
                currentMeshRenderer->IndexCount = foundMesh->second.IndexCount;
                currentMeshRenderer->VertexBuffer = foundMesh->second.VertexBuffer;
            }

            // reapply material if it's changed
            if (previouslyActiveMaterialPos != materialPos)
            {
                previouslyActiveMaterialPos = materialPos;

                auto loadedMaterialData = static_cast<char*>(SkipHeader(currentMaterial->Header));

                // load material uniforms
                Assets::ConfiguredUniform* vertUniforms = (Assets::ConfiguredUniform*)(loadedMaterialData + currentMaterial->VertUniformOffset);
                for (size_t i = 0; i < currentMaterial->VertUniformCount; i++)
                {
                    Assets::ConfiguredUniform materialUniform = vertUniforms[i];
                    SDL_PushGPUVertexUniformData(
                        commandBuffer, 
                        materialUniform.Binding, 
                        &materialUniform.Data.Data, 
                        (uint32_t)Core::Pipeline::GetVariantPayloadSize(materialUniform.Data));
                }

                Assets::ConfiguredUniform* fragUniforms = (Assets::ConfiguredUniform*)(loadedMaterialData + currentMaterial->FragUniformOffset);
                for (size_t i = 0; i < currentMaterial->FragUniformCount; i++)
                {
                    Assets::ConfiguredUniform materialUniform = fragUniforms[i];
                    SDL_PushGPUFragmentUniformData(
                        commandBuffer, 
                        materialUniform.Binding, 
                        &materialUniform.Data.Data, 
                        (uint32_t)Core::Pipeline::GetVariantPayloadSize(materialUniform.Data));
                }
            }

            // load the MVP, skip if the renderer has no spatial relation
            auto foundModelSpatialRelation = services->ModuleManager->GetRootModule()->SpatialComponents.find(currentMeshRenderer->Entity);
            if (foundModelSpatialRelation == services->ModuleManager->GetRootModule()->SpatialComponents.end())
                continue;

            // compute MVP from scene components
            glm::mat4 modelMatrix = foundModelSpatialRelation->second.Transform();

            // bind dynamic injections (only uniforms rn)
            auto dynamicVertexUniforms = (Assets::InjectedUniform*)(loadedPipelineData + currentPipeline->DynamicVertUniform.Offset);
            for (size_t i = 0; i < currentPipeline->DynamicVertUniform.Count; i++)
            {
                Assets::InjectedUniform uniform = dynamicVertexUniforms[i];
                switch (uniform.Identifier)
                {
                case (unsigned char)Engine::Extension::RendererModule::Assets::DynamicUniformIdentifier::ModelTransform:
                    SDL_PushGPUVertexUniformData(commandBuffer, uniform.Binding, &modelMatrix, sizeof(modelMatrix));
                    break;
                case (unsigned char)Engine::Extension::RendererModule::Assets::DynamicUniformIdentifier::ViewTransform:
                    SDL_PushGPUVertexUniformData(commandBuffer, uniform.Binding, &viewMatrix, sizeof(viewMatrix));
                    break;
                case (unsigned char)Engine::Extension::RendererModule::Assets::DynamicUniformIdentifier::ProjectionTransform:
                    SDL_PushGPUVertexUniformData(commandBuffer, uniform.Binding, &projectMatrix, sizeof(projectMatrix));
                    break;
                }
            }

            auto dynamicFragmentUniforms = (Assets::InjectedUniform*)(loadedPipelineData + currentPipeline->DynamicFragUniform.Offset);
            for (size_t i = 0; i < currentPipeline->DynamicFragUniform.Count; i++)
            {
                Assets::InjectedUniform uniform = dynamicFragmentUniforms[i];
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
            SDL_GPUBufferBinding vboBinding{currentMeshRenderer->VertexBuffer, 0};
            SDL_BindGPUVertexBuffers(pass, 0, &vboBinding, 1);
            SDL_GPUBufferBinding iboBinding{currentMeshRenderer->IndexBuffer, 0};
            SDL_BindGPUIndexBuffer(pass, &iboBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
            
            // draw
            SDL_DrawGPUIndexedPrimitives(pass, currentMeshRenderer->IndexCount, 1, 0, 0, 0);
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
            HASH_NAME("VertexShader"),
            Assets::ContextualizeVertexShader,
            Assets::IndexVertexShader
        },
        {
            HASH_NAME("FragmentShader"),
            Assets::ContextualizeFragmentShader,
            Assets::IndexFragmentShader
        },
        {
            HASH_NAME("SlangVertexShader"),
            Assets::ContextualizeVertexShader,
            Assets::IndexVertexShader
        },
        {
            HASH_NAME("SlangFragmentShader"),
            Assets::ContextualizeFragmentShader,
            Assets::IndexFragmentShader
        },
        {
            HASH_NAME("RenderPipeline"),
            Assets::ContextualizeRenderPipeline,
            Assets::IndexRenderPipeline
        },
        {
            HASH_NAME("Material"),
            Assets::ContextualizeMaterial,
            Assets::IndexMaterial
        },
        {
            HASH_NAME("Mesh"),
            Assets::ContextualizeStaticMesh,
            Assets::IndexStaticMesh,
        }
    };

    static const Core::Pipeline::SynchronousCallback Callbacks[]
    {
        {
            Core::Pipeline::SynchronousCallbackStage::Render,
            RenderUpdate
        }
    };

    static const Core::Pipeline::ComponentDefinition Components[]
    {
        {
            HASH_NAME("MeshRenderer"),
            Components::CompileMeshRenderer,
            Components::LoadMeshRenderer
        },
        {
            HASH_NAME("DirectionalLight"),
            Components::CompileDirectionalLight,
            Components::LoadDirectionalLight
        }
    };

    return Core::Pipeline::ModuleDefinition 
    {
        HASH_NAME("RendererModule"),
        InitRendererModule,
        DisposeRendererModule,
        Assets,
        sizeof(Assets) / sizeof(Core::Pipeline::AssetDefinition),
        Callbacks,
        sizeof(Callbacks) / sizeof(Core::Pipeline::SynchronousCallback),
        nullptr,
        0,
        Components,
        sizeof(Components) / sizeof(Core::Pipeline::ComponentDefinition)
    };
}