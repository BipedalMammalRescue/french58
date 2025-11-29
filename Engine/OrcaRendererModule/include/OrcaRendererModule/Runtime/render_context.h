#pragma once

#include "OrcaRendererModule/Assets/render_graph.h"
#include "OrcaRendererModule/Assets/shader_effect.h"

namespace Engine::Extension::OrcaRendererModule::Runtime {

enum class RenderCommandType
{
    BeginShaderPass,
    EndShaderPass
};

struct BeginShaderPassCommand
{
    Assets::RenderPass* Definition;
};

struct RenderCommand 
{
    size_t SortKey;

    RenderCommandType Type;
    
    union {
        BeginShaderPassCommand BeginShaderPass;
    };
};

class RenderContext
{
private:
    // TODO: temp implementation, the real one should directly pump data into the concurrent queue
    std::vector<RenderCommand> m_Commands;

public:
    void PopulateCommandForRenderGraph(Assets::RenderGraphHeader* graph);
    void PopulateCommandForShader(Assets::ShaderHeader* shader, size_t shaderIndex);

    // TODO: populate command for material
    // TODO: populate command for object
};

}