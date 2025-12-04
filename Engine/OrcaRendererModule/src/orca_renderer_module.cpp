#include "OrcaRendererModule/orca_renderer_module.h"
#include "EngineCore/Containers/Uniform/sorted_array.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "OrcaRendererModule/Assets/shader.h"
#include "OrcaRendererModule/Runtime/render_context.h"

#include "SDL3/SDL_stdinc.h"

using namespace Engine::Extension::OrcaRendererModule;

void ModuleState::PopulateRenderCommands(Runtime::RenderContext *context)
{
    for (size_t i = 0; i < SDL_arraysize(m_RenderGraphs); i++)
    {
        if (!m_RenderGraphs[i].Valid)
            continue;

        context->PopulateCommandForRenderGraph(m_RenderGraphs[i].Graph);
    }

    for (size_t materialIndex = 0; materialIndex < m_Materials.GetCount(); materialIndex++)
    {
        // potential optimization opportunity here: cache the referenced shader in each material so we don't have to
        // seek on ever frame
        Assets::Material *material = m_Materials.PtrAt(materialIndex)->Value;

        // look for a shader
        Core::Containers::Uniform::AnnotatedNode<Core::Pipeline::HashId, Assets::Shader *> shaderSearchKey{
            .Key = material->ShaderName};
        size_t shaderIndex = m_Shaders.Search(shaderSearchKey);
        if (shaderIndex >= m_Shaders.GetCount())
            continue;
        Assets::Shader *shader = m_Shaders.PtrAt(shaderIndex)->Value;

        // loop through the shader passes and use the graph information
        for (size_t effectIndex = 0; effectIndex < shader->EffectCount; effectIndex++)
        {
            Assets::ShaderEffect *effect = &shader->GetShaderEffects()[effectIndex];

            if (!effect->RenderGraphCache.Attempted)
            {
                RenderGraphContainer searchKey{.Name = effect->RenderGraphName};
                RenderGraphContainer *targetGraph =
                    (RenderGraphContainer *)SDL_bsearch(&searchKey, m_RenderGraphs, SDL_arraysize(m_RenderGraphs),
                                                        sizeof(RenderGraphContainer), [](const void *a, const void *b) {
                                                            const RenderGraphContainer *lhs = (RenderGraphContainer *)a;
                                                            const RenderGraphContainer *rhs = (RenderGraphContainer *)b;
                                                            if (lhs->Name > rhs->Name)
                                                                return 1;
                                                            else if (lhs->Name < rhs->Name)
                                                                return -1;
                                                            return 0;
                                                        });
                effect->RenderGraphCache.Graph = targetGraph->Graph;
                effect->RenderGraphCache.Attempted = true;
            }

            if (effect->RenderGraphCache.Graph == nullptr)
                continue;

            context->PopulateCommandForMaterial(effect->RenderGraphCache.Graph, material, materialIndex, effect,
                                                shaderIndex);
        }
    }

    for (size_t shaderIndex = 0; shaderIndex < m_Shaders.GetCount(); shaderIndex++)
    {
        Assets::Shader *shader = m_Shaders.PtrAt(shaderIndex)->Value;

        for (size_t effectIndex = 0; effectIndex < shader->EffectCount; effectIndex++)
        {
            Assets::ShaderEffect *effect = &shader->GetShaderEffects()[effectIndex];

            if (!effect->RenderGraphCache.Attempted)
                continue;
            effect->RenderGraphCache.Attempted = false;

            if (effect->RenderGraphCache.Graph == nullptr)
                continue;

            context->PopulateCommandForShaderEffect(effect->RenderGraphCache.Graph, effect, shaderIndex);
        }
    }

    // generate commands for objects
}
