#include "OrcaRendererModule/Runtime/render_context.h"
#include "OrcaRendererModule/Assets/render_graph.h"
#include "OrcaRendererModule/Assets/shader_effect.h"
#include <cassert>
#include <cstdint>

enum class RenderPassRelation : char
{
    Begin,
    Use,
    End
};

// we have an implicit hard limit of 65536 shaders and 65536 materials in totoal
// in the future probably use relative indexing so we can shrink down the width
// note that anything in the object level should come in a package instead of sorted commands since there's no reason to sort them
static size_t CreateSortKey(char graphBits, char passBits, RenderPassRelation passRelation, uint16_t shaderBits, uint16_t materialBits)
{
    assert(graphBits < 16 && graphBits >= 0);
    assert(passBits < 16 && passBits >= 0);

    size_t result = 0;

    // graph and pass form a 8 bit integer
    result += graphBits;
    result <<= 4;
    result += passBits;

    // pass relation takes up 2 bits
    result <<= 2;
    result += (char)passRelation;

    // shader and material form a 32 bit integer
    result <<= 16;
    result += shaderBits;
    result <<= 16;
    result += materialBits;

    return result;
}

void Engine::Extension::OrcaRendererModule::Runtime::RenderContext::PopulateCommandForRenderGraph(
    Assets::RenderGraphHeader* graph)
{
    for (size_t passIndex = 0; passIndex < graph->RenderPassCount && passIndex < 16; passIndex++)
    {
        Assets::RenderPass* pass = graph->GetRenderPasses() + passIndex;

        // command that creates the render pass
        size_t beginCommandSortKey = CreateSortKey(
            graph->Altitude, passIndex, RenderPassRelation::Begin, 0, 0);

        m_Commands.push_back({
            .SortKey = beginCommandSortKey,
            .Type = RenderCommandType::BeginShaderPass,
            .BeginShaderPass = {
                pass
            }
        });

        // command that ends the render pass
        size_t endCommandSortKey = CreateSortKey(
            graph->Altitude, passIndex, RenderPassRelation::End, 0, 0);

        m_Commands.push_back({
            .SortKey = endCommandSortKey,
            .Type = RenderCommandType::EndShaderPass
        });
    }
}

void Engine::Extension::OrcaRendererModule::Runtime::RenderContext::PopulateCommandForShader(Assets::ShaderHeader* shader, size_t shaderIndex)
{
    for (size_t effectIndex = 0; effectIndex < shader->EffectCount; effectIndex ++)
    {
        Assets::ShaderEffect* effect = shader->GetShaderEffects() + effectIndex;

        // we assume the validity of the render pass referenced by this shader has already been checked
        size_t sortKey = CreateSortKey(
            effect->RenderGraph->Altitude, effect->RenderPassId, RenderPassRelation::Use, shaderIndex, 0);

        // TODO: based on static bindings in the shader, bind parameters
        // TODO: does the shader binding command also arrive in the form of a package?
    }
}

// TODO: note to self: when sorting materials the actual commands that use the material needs to be added by one to make sure 