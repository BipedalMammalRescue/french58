#pragma once

#include "EngineCore/Logging/logger.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "OrcaRendererModule/Assets/render_graph.h"
#include "OrcaRendererModule/Assets/shader_effect.h"

namespace Engine::Extension::OrcaRendererModule {

// NOTE: we use liberally fixed-size arrays for things that don't have a reason to be long

struct SceneObject
{
    Core::Pipeline::HashId RenderGraphName;
    Core::Pipeline::HashId MaterialName;
    Core::Pipeline::HashId MeshName;

    // something needs to store skeleton and animation
};

class ModuleState 
{
private:
    Core::Logging::Logger m_Logger;

    // ???
    std::vector<SceneObject> m_SceneNodes;

    // maximum 16 render graph slots (refcounted)
    struct RenderGraphContainer
    {
        Assets::RenderGraphHeader* Graph;
        bool Valid;
    };
    RenderGraphContainer m_RenderGraphs[16];

public:
    inline Core::Logging::Logger* GetLogger() { return &m_Logger; }
    
    Assets::RenderGraphHeader* FindGraph(const Core::Pipeline::HashId& name);
    Assets::ShaderHeader* FindShader(const Core::Pipeline::HashId& name);
};

}