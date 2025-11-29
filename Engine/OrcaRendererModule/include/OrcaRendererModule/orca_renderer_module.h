#pragma once

#include "EngineCore/Logging/logger.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "OrcaRendererModule/Assets/render_graph.h"
#include "OrcaRendererModule/Assets/shader_effect.h"
#include <unordered_map>

namespace Engine::Extension::OrcaRendererModule {

// NOTE: we use liberally fixed-size arrays for things that don't have a reason to be long

class ModuleState 
{
private:
    Core::Logging::Logger m_Logger;

    // no limit on the number of shaders
    std::vector<Assets::ShaderHeader*> m_LoadedShaders;
    std::unordered_map<Core::Pipeline::HashId, uint32_t> m_ShaderIndex;

    // maximum 16 render graph slots (refcounted)
    struct RenderGraphContainer
    {
        Assets::RenderGraphHeader* Graph;
        bool Valid;
        uint32_t RefCount;
    };
    RenderGraphContainer m_RenderGraphs[16];

public:
    inline Core::Logging::Logger* GetLogger() { return &m_Logger; }

    void PopulateDrawCalls();
};

}