#include "OrcaRendererModule/orca_renderer_module.h"
#include "OrcaRendererModule/Assets/material.h"
#include "OrcaRendererModule/Assets/render_graph.h"
#include "OrcaRendererModule/Assets/shader.h"
#include "OrcaRendererModule/Components/static_mesh_renderer.h"
#include "OrcaRendererModule/Runtime/render_context.h"

using namespace Engine::Extension::OrcaRendererModule;

void ModuleState::PopulateRenderCommands(Runtime::RenderContext *context)
{
    for (size_t i = 0; i < m_RenderGraphs.GetCount(); i++)
    {
        context->PopulateCommandForRenderGraph(m_RenderGraphs.Get(i));
    }

    for (size_t materialIndex = 0; materialIndex < m_Materials.GetCount(); materialIndex++)
    {
        // potential optimization opportunity here: cache the referenced shader in each material so we don't have to
        // seek on ever frame
        Assets::Material *material = m_Materials.Get(materialIndex);

        // look for a shader
        Assets::Shader *shader = m_Shaders.Get(material->ShaderRef);
        if (shader == nullptr)
            continue;

        // loop through the shader passes and use the graph information
        for (size_t effectIndex = 0; effectIndex < shader->EffectCount; effectIndex++)
        {
            Assets::ShaderEffect *effect = &shader->GetShaderEffects()[effectIndex];
            size_t graphRef = shader->GetRenderGraphReferences()[effectIndex];

            Assets::RenderGraph *graph = m_RenderGraphs.Get(graphRef);
            if (graph == nullptr)
                continue;

            context->PopulateCommandForMaterial(graph, material, materialIndex, effect, material->ShaderRef);
        }
    }

    for (size_t shaderIndex = 0; shaderIndex < m_Shaders.GetCount(); shaderIndex++)
    {
        Assets::Shader *shader = m_Shaders.Get(shaderIndex);

        for (size_t effectIndex = 0; effectIndex < shader->EffectCount; effectIndex++)
        {
            Assets::ShaderEffect *effect = &shader->GetShaderEffects()[effectIndex];
            size_t graphRef = shader->GetRenderGraphReferences()[effectIndex];

            Assets::RenderGraph *graph = m_RenderGraphs.Get(graphRef);
            if (graph == nullptr)
                continue;

            context->PopulateCommandForShaderEffect(graph, effect, shaderIndex);
        }
    }

    // generate commands for objects
    for (size_t staticMeshIndex = 0; staticMeshIndex < m_StaticMeshRenderers.GetCount(); staticMeshIndex++)
    {
        Core::Containers::Uniform::AnnotatedNode<int, Components::StaticMeshRenderer *> *rendererComponent =
            m_StaticMeshRenderers.PtrAt(staticMeshIndex);

        Assets::Mesh *mesh = m_Meshes.Get(rendererComponent->Value->MeshRef);
        if (mesh == nullptr)
            continue;

        Assets::Material *material = m_Materials.Get(rendererComponent->Value->MaterialRef);
        Assets::RenderGraph *graph = m_RenderGraphs.Get(rendererComponent->Value->RenderGraphRef);

        if (material == nullptr || graph == nullptr)
            continue;

        Assets::Shader *shader = m_Shaders.Get(material->ShaderRef);
        if (shader == nullptr)
            continue;

        // sign it up for every render pass it participates
        for (size_t effectIndex = 0; effectIndex < shader->EffectCount; effectIndex++)
        {
            Assets::ShaderEffect *effect = &shader->GetShaderEffects()[effectIndex];
            size_t graphRef = shader->GetRenderGraphReferences()[effectIndex];

            if (graphRef != rendererComponent->Value->RenderGraphRef)
                continue;

            Assets::RenderGraph *graph = m_RenderGraphs.Get(graphRef);
            if (graph == nullptr || effect->RenderPassId >= graph->RenderPassCount)
                continue;

            Assets::RenderPass *pass = &graph->GetRenderPasses()[effect->RenderPassId];

            context->PopulateCommandForObject(graph, effect->RenderPassId, rendererComponent->Value->MaterialRef,
                                              material->ShaderRef, mesh, rendererComponent->Value->DataCollection);
        }
    }
}