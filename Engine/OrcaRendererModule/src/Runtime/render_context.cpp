#include "OrcaRendererModule/Runtime/render_context.h"
#include "OrcaRendererModule/Assets/material.h"
#include "OrcaRendererModule/Assets/mesh.h"
#include "OrcaRendererModule/Assets/render_graph.h"
#include "OrcaRendererModule/Assets/shader.h"
#include "OrcaRendererModule/Runtime/renderer_resource.h"

#include "SDL3/SDL_stdinc.h"

#include <cassert>

using namespace Engine::Extension::OrcaRendererModule::Runtime;

enum class RenderPassRelation : unsigned char
{
    Begin,
    BindShader,
    BindMaterial,
    Draw,
    Count
};

class SortKeyGenerator
{
private:
    size_t m_Key = 0;

public:
    void Push(size_t value, size_t offset)
    {
        m_Key *= offset;
        m_Key += value;
    }

    size_t Get() const
    {
        return m_Key;
    }
};

// we have an implicit hard limit of 65536 shaders and 65536 materials in totoal
// in the future probably use relative indexing so we can shrink down the width
// note that anything in the object level should come in a package instead of sorted commands since
// there's no reason to sort them
static size_t CreateSortKey(size_t graphIndex, size_t graphCount, size_t passIndex,
                            size_t maxPassCount, RenderPassRelation passRelation,
                            size_t maxPassRelation, size_t shaderIndex, size_t shaderCount,
                            size_t materialIndex, size_t materialCount)
{
    SortKeyGenerator gen;
    gen.Push(graphIndex, graphCount);
    gen.Push(passIndex, maxPassCount);
    gen.Push(maxPassRelation, maxPassCount);
    gen.Push(shaderIndex, shaderCount);
    gen.Push(materialIndex, materialCount);

    return gen.Get();
}

void Engine::Extension::OrcaRendererModule::Runtime::RenderContext::PopulateCommandForRenderGraph(
    Assets::RenderGraph *graph)
{
    for (size_t passIndex = 0; passIndex < graph->RenderPassCount && passIndex < 16; passIndex++)
    {
        Assets::RenderPass *pass = graph->GetRenderPasses() + passIndex;

        // command that creates the render pass
        size_t beginCommandSortKey =
            CreateSortKey(graph->Altitude, m_MaxGraphAltitude, passIndex, m_MaxPassCount,
                          RenderPassRelation::Begin, (size_t)RenderPassRelation::Count, 0,
                          m_ShaderCount, 0, m_MaterialCount);
        m_Commands.push_back({.SortKey = beginCommandSortKey,
                              .Type = RenderCommandType::BeginRenderPass,
                              .BeginShaderPass = {pass}});
    }
}

void Engine::Extension::OrcaRendererModule::Runtime::RenderContext::PopulateCommandForShaderEffect(
    Assets::RenderGraph *targetGraph, Assets::ShaderEffect *effect, size_t shaderIndex)
{
    // we have an arbitrary count limit for shaders due to bit width; at a time only ushort_max
    // shaders can be loaded in memory
    assert(shaderIndex < 65536);

    // shader binding takes up a special relation in the sort key, and a shader can only define one
    // effect for every shader pass
    size_t sortKey = CreateSortKey(targetGraph->Altitude, m_MaxGraphAltitude, effect->RenderPassId,
                                   m_MaxPassCount, RenderPassRelation::BindShader,
                                   (size_t)RenderPassRelation::Count, shaderIndex, m_ShaderCount, 0,
                                   m_MaterialCount);

    m_Commands.push_back(
        {.SortKey = sortKey, .Type = RenderCommandType::BindShader, .BindShader = {effect}});
}

void Engine::Extension::OrcaRendererModule::Runtime::RenderContext::PopulateCommandForMaterial(
    Assets::RenderGraph *targetGraph, Assets::Material *material, size_t materialIndex,
    Assets::ShaderEffect *effect, size_t shaderIndex)
{
    size_t sortKey = CreateSortKey(targetGraph->Altitude, m_MaxGraphAltitude, effect->RenderPassId,
                                   m_MaxPassCount, RenderPassRelation::BindMaterial,
                                   (size_t)RenderPassRelation::Count, shaderIndex, m_ShaderCount,
                                   materialIndex, m_MaterialCount);

    m_Commands.push_back(
        {.SortKey = sortKey, .Type = RenderCommandType::BindMaterial, .BindMaterial = {material}});
}

void Engine::Extension::OrcaRendererModule::Runtime::RenderContext::PopulateCommandForObject(
    Assets::RenderGraph *targetGraph, size_t renderPassId, size_t materialIndex, size_t shaderIndex,
    Assets::Mesh *mesh, size_t resourceCount, NamedRendererResource *objectResources)
{
    size_t sortKey =
        CreateSortKey(targetGraph->Altitude, m_MaxGraphAltitude, renderPassId, m_MaxPassCount,
                      RenderPassRelation::Draw, (size_t)RenderPassRelation::Count, shaderIndex,
                      m_ShaderCount, materialIndex, m_MaterialCount);

    m_Commands.push_back({.SortKey = sortKey,
                          .Type = RenderCommandType::Draw,
                          .Draw = {mesh, resourceCount, objectResources}});
}

void Engine::Extension::OrcaRendererModule::Runtime::RenderContext::FinalizeCommands()
{
    // TODO: this would need to be moved to the renderer implementation
    // sort all commands
    SDL_qsort(m_Commands.data(), m_Commands.size(), sizeof(RenderCommand),
              [](const void *a, const void *b) {
                  const RenderCommand *lhs = (RenderCommand *)a;
                  const RenderCommand *rhs = (RenderCommand *)b;

                  if (lhs->SortKey > rhs->SortKey)
                      return 1;
                  else if (lhs->SortKey < rhs->SortKey)
                      return -1;
                  return 0;
              });
}

void RenderContext::Clear()
{
    m_Commands.clear();
}