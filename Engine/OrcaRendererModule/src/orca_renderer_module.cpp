#include "OrcaRendererModule/orca_renderer_module.h"
#include "OrcaRendererModule/Assets/material.h"
#include "OrcaRendererModule/Assets/render_graph.h"
#include "OrcaRendererModule/Assets/shader.h"
#include "OrcaRendererModule/Components/static_mesh_renderer.h"
#include "OrcaRendererModule/Runtime/render_context.h"

using namespace Engine::Extension::OrcaRendererModule;

void ModuleState::PopulateRenderCommands(Runtime::RenderContext *context)
{
    for (size_t i = 0; i < RenderGraphs.GetCount(); i++)
    {
        context->PopulateCommandForRenderGraph(RenderGraphs.Get(i));
    }

    for (size_t materialIndex = 0; materialIndex < Materials.GetCount(); materialIndex++)
    {
        // potential optimization opportunity here: cache the referenced shader in each material so
        // we don't have to seek on ever frame
        Assets::Material *material = Materials.Get(materialIndex);

        // look for a shader
        Assets::Shader *shader = Shaders.Get(material->ShaderRef);
        if (shader == nullptr)
            continue;

        // loop through the shader passes and use the graph information
        for (size_t effectIndex = 0; effectIndex < shader->EffectCount; effectIndex++)
        {
            Assets::ShaderEffect *effect = &shader->GetShaderEffects()[effectIndex];
            size_t graphRef = effect->RenderGraphReference;

            Assets::RenderGraph *graph = RenderGraphs.Get(graphRef);
            if (graph == nullptr)
                continue;

            context->PopulateCommandForMaterial(graph, material, materialIndex, effect,
                                                material->ShaderRef);
        }
    }

    for (size_t shaderIndex = 0; shaderIndex < Shaders.GetCount(); shaderIndex++)
    {
        Assets::Shader *shader = Shaders.Get(shaderIndex);

        for (size_t effectIndex = 0; effectIndex < shader->EffectCount; effectIndex++)
        {
            Assets::ShaderEffect *effect = &shader->GetShaderEffects()[effectIndex];
            size_t graphRef = effect->RenderGraphReference;

            Assets::RenderGraph *graph = RenderGraphs.Get(graphRef);
            if (graph == nullptr)
                continue;

            context->PopulateCommandForShaderEffect(graph, effect, shaderIndex);
        }
    }

    // generate commands for objects
    for (size_t staticMeshIndex = 0; staticMeshIndex < StaticMeshRenderers.GetCount();
         staticMeshIndex++)
    {
        Core::Containers::Uniform::AnnotatedNode<int, Components::StaticMeshRenderer *>
            *rendererComponent = StaticMeshRenderers.PtrAt(staticMeshIndex);

        Assets::Mesh mesh = Meshes.Get(rendererComponent->Value->MeshRef);

        Assets::Material *material = Materials.Get(rendererComponent->Value->MaterialRef);
        Assets::RenderGraph *graph = RenderGraphs.Get(rendererComponent->Value->RenderGraphRef);

        if (material == nullptr || graph == nullptr)
            continue;

        Assets::Shader *shader = Shaders.Get(material->ShaderRef);
        if (shader == nullptr)
            continue;

        // sign it up for every render pass it participates
        for (size_t effectIndex = 0; effectIndex < shader->EffectCount; effectIndex++)
        {
            Assets::ShaderEffect *effect = &shader->GetShaderEffects()[effectIndex];
            size_t graphRef = effect->RenderGraphReference;

            if (graphRef != rendererComponent->Value->RenderGraphRef)
                continue;

            Assets::RenderGraph *graph = RenderGraphs.Get(graphRef);
            if (graph == nullptr || effect->RenderPassId >= graph->RenderPassCount)
                continue;

            Assets::RenderPass *pass = &graph->GetRenderPasses()[effect->RenderPassId];

            context->PopulateCommandForObject(
                graph, effect->RenderPassId, rendererComponent->Value->MaterialRef,
                material->ShaderRef, mesh, 1, &rendererComponent->Value->MvpIndex);
        }
    }
}