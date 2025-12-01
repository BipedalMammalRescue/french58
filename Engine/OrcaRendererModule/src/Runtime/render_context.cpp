#include "OrcaRendererModule/Runtime/render_context.h"
#include "OrcaRendererModule/Assets/render_graph.h"
#include "OrcaRendererModule/Assets/shader_effect.h"
#include <cassert>
#include <cstdint>

enum class RenderPassRelation : unsigned char
{
    BeginPass,
    BindShader,
    Draw,
    End
};

class SortKeyGenerator
{
private:
    size_t m_Key = 0;
    int m_Width = 0;

public:
    void Push(int value, int width)
    {
        assert(m_Width + width <= 64);
        m_Width += width;

        m_Key <<= width;
        m_Key += value & ((1 << (width + 1)) - 1);
    }

    size_t Get() const
    {
        return m_Key;
    }
};

// we have an implicit hard limit of 65536 shaders and 65536 materials in totoal
// in the future probably use relative indexing so we can shrink down the width
// note that anything in the object level should come in a package instead of sorted commands since there's no reason to sort them
static size_t CreateSortKey(char graphBits, char passBits, RenderPassRelation passRelation, uint16_t shaderBits, uint16_t materialBits)
{
    assert(graphBits < 16 && graphBits >= 0);
    assert(passBits < 16 && passBits >= 0);

    SortKeyGenerator key;

    // graph and pass form a 8 bit integer
    key.Push(graphBits, 4);
    key.Push(passBits, 4);

    // pass relation takes up 2 bits
    key.Push((unsigned char)passRelation, 2);

    // shader and material form a 32 bit integer
    key.Push(shaderBits, 16);
    key.Push(materialBits, 16);

    return key.Get();
}

void Engine::Extension::OrcaRendererModule::Runtime::RenderContext::PopulateCommandForRenderGraph(
    Assets::RenderGraphHeader* graph)
{
    for (size_t passIndex = 0; passIndex < graph->RenderPassCount && passIndex < 16; passIndex++)
    {
        Assets::RenderPass* pass = graph->GetRenderPasses() + passIndex;

        // command that creates the render pass
        size_t beginCommandSortKey = CreateSortKey(
            graph->Altitude, 
            passIndex, 
            RenderPassRelation::BeginPass, 
            0, 0);

        m_Commands.push_back({
            .SortKey = beginCommandSortKey,
            .Type = RenderCommandType::BeginRenderPass,
            .BeginShaderPass = {
                pass
            }
        });

        // command that ends the render pass
        size_t endCommandSortKey = CreateSortKey(
            graph->Altitude, passIndex, RenderPassRelation::End, 0, 0);

        m_Commands.push_back({
            .SortKey = endCommandSortKey,
            .Type = RenderCommandType::EndRenderPass
        });
    }
}

void Engine::Extension::OrcaRendererModule::Runtime::RenderContext::PopulateCommandForShader(Assets::ShaderHeader* shader, size_t shaderIndex)
{
    for (size_t effectIndex = 0; effectIndex < shader->EffectCount; effectIndex ++)
    {
        Assets::ShaderEffect* effect = shader->GetShaderEffects() + effectIndex;

        // find the render graph
        Assets::RenderGraphHeader* targetGraph = m_Module->FindGraph(effect->RenderGraphName);
        if (targetGraph == nullptr)
            continue;
        
        // check render graph states
        if (effect->RenderPassId >= targetGraph->RenderPassCount)
            continue;

        // shader binding takes up a special relation in the sort key
        size_t sortKey = CreateSortKey(
            targetGraph->Altitude, 
            effect->RenderPassId, 
            RenderPassRelation::BindShader, 
            shaderIndex, 0);

        m_Commands.push_back({
            .SortKey = sortKey,
            .Type = RenderCommandType::BindShader,
            .BindShader = {
                effect
            }
        });
    }
}