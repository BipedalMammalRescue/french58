#pragma once

#include "OrcaRendererModule/Assets/material.h"
#include "OrcaRendererModule/Assets/mesh.h"
#include "OrcaRendererModule/Assets/render_graph.h"
#include "OrcaRendererModule/Assets/shader.h"
#include "OrcaRendererModule/Runtime/renderer_resource.h"
#include "OrcaRendererModule/orca_renderer_module.h"

namespace Engine::Extension::OrcaRendererModule::Runtime {

enum class RenderCommandType
{
    BeginRenderPass,
    BindShader,
    BindMaterial,
    Draw,
};

// Note: render commands are sorted then executed on a renderer executor (maybe we can have more
// renderer executors when we have multiple graphs active?). Render commands are also *stateful*,
// therefore when a material is bound it encodes the shader into its sort key and only pass the
// resources it carries as command data. Same statefule command handling for rendered objects, that
// they won't contain any information about their material and assumes that any information not
// related to the object directly has been bound already.
struct RenderCommand
{
    size_t SortKey;

    RenderCommandType Type;

    union {
        struct
        {
            Assets::RenderPass *Definition;
        } BeginShaderPass;

        struct
        {
            Assets::ShaderEffect *ShaderEffect;
        } BindShader;

        struct
        {
            Assets::Material *Material;
        } BindMaterial;

        struct
        {
            Assets::Mesh *Mesh;
            size_t ResourceCount;
            NamedRendererResource *Resources;
        } Draw;
    };
};

class RenderContext
{
private:
    // TODO: temp implementation, the real one should directly pump data into the concurrent queue
    std::vector<RenderCommand> m_Commands;
    ModuleState *m_Module;

    size_t m_MaxGraphAltitude;
    size_t m_MaxPassCount;
    size_t m_ShaderCount;
    size_t m_MaterialCount;

public:
    void PopulateCommandForRenderGraph(Assets::RenderGraph *graph);

    void PopulateCommandForShaderEffect(Assets::RenderGraph *graph, Assets::ShaderEffect *effect,
                                        size_t shaderIndex);

    // TODO: material commands also need information from graphs and such, what if we just generate
    // material commands and have the renderer figure out when it needs to parse the render graph
    // data? From the caller's side this multi-layered memory seeking is very time consuming

    void PopulateCommandForMaterial(Assets::RenderGraph *targetGraph, Assets::Material *material,
                                    size_t materialIndex, Assets::ShaderEffect *effect,
                                    size_t shaderIndex);

    void PopulateCommandForObject(Assets::RenderGraph *targetGraph, size_t renderPassId,
                                  size_t materialIndex, size_t shaderIndex, Assets::Mesh *mesh,
                                  size_t objectResCount, NamedRendererResource *objectResources);

    void FinalizeCommands();
    void Clear();
};

} // namespace Engine::Extension::OrcaRendererModule::Runtime