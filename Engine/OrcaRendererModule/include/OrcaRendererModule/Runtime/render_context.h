#pragma once

#include "OrcaRendererModule/Assets/render_graph.h"
#include "OrcaRendererModule/Assets/shader_effect.h"
#include "OrcaRendererModule/orca_renderer_module.h"

namespace Engine::Extension::OrcaRendererModule::Runtime {

enum class RenderCommandType
{
    BeginRenderPass,
    EndRenderPass,
    BindShader
};

struct BeginShaderPassCommand
{
    Assets::RenderPass* Definition;
};

struct BindShaderCommand
{
    Assets::ShaderEffect* ShaderEffect;
};

struct RenderCommand 
{
    size_t SortKey;

    RenderCommandType Type;
    
    union {
        BeginShaderPassCommand BeginShaderPass;
        BindShaderCommand BindShader;
    };
};

class RenderContext
{
private:
    // TODO: temp implementation, the real one should directly pump data into the concurrent queue
    std::vector<RenderCommand> m_Commands;
    ModuleState* m_Module;

public:
    void PopulateCommandForRenderGraph(Assets::RenderGraphHeader* graph);
    void PopulateCommandForShader(Assets::ShaderHeader* shader, size_t shaderIndex);
    void PopulateCommandForObject();

    // TODO: populate command for material
    // TODO: populate command for object
};

}