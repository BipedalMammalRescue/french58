#pragma once

#include "EngineCore/Containers/Uniform/sorted_array.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "OrcaRendererModule/Assets/material.h"
#include "OrcaRendererModule/Assets/render_graph.h"
#include "OrcaRendererModule/Assets/shader.h"
#include "OrcaRendererModule/Components/static_mesh_renderer.h"

namespace Engine::Extension::OrcaRendererModule {

namespace Runtime {
class RenderContext;
}

// NOTE: we use liberally fixed-size arrays for things that don't have a reason to be long

class ModuleState
{
private:
    Core::Logging::Logger m_Logger;

    // maximum 16 render graph slots (refcounted)
    struct RenderGraphContainer
    {
        Core::Pipeline::HashId Name;
        Assets::RenderGraphHeader *Graph;
        bool Valid;
    };
    RenderGraphContainer m_RenderGraphs[16];

    // since the renderer doesn't ever need to reference the shaders by their index, this sorted array is never visible
    // to the renderer maximum 65536
    Core::Containers::Uniform::AnnotationSortedArray<Core::Pipeline::HashId, Assets::Shader *> m_Shaders;

    // since the renderer doesn't ever need to reference the material by their index, this sorted array is never visible
    // to the renderer maximum 65536
    Core::Containers::Uniform::AnnotationSortedArray<Core::Pipeline::HashId, Assets::Material *> m_Materials;

    Core::Containers::Uniform::AnnotationSortedArray<int, Components::StaticMeshRenderer *> m_StaticMeshRenderers;

public:
    inline Core::Logging::Logger *GetLogger()
    {
        return &m_Logger;
    }
    void PopulateRenderCommands(Runtime::RenderContext *context);
};

} // namespace Engine::Extension::OrcaRendererModule