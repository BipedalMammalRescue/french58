#pragma once

#include "EngineCore/Containers/Uniform/sorted_array.h"
#include "EngineCore/Logging/logger.h"
#include "OrcaRendererModule/Assets/material.h"
#include "OrcaRendererModule/Assets/mesh.h"
#include "OrcaRendererModule/Assets/render_graph.h"
#include "OrcaRendererModule/Assets/shader.h"
#include "OrcaRendererModule/Components/static_mesh_renderer.h"
#include "OrcaRendererModule/Runtime/reference_list.h"

namespace Engine::Extension::OrcaRendererModule {

namespace Runtime {
class RenderContext;
}

// NOTE: we use liberally fixed-size arrays for things that don't have a reason to be long

class ModuleState
{
private:
    Core::Logging::Logger m_Logger;

    Runtime::ReferenceList<Assets::RenderGraph> m_RenderGraphs;
    Runtime::ReferenceList<Assets::Shader> m_Shaders;
    Runtime::ReferenceList<Assets::Material> m_Materials;

    Runtime::ReferenceList<Assets::Mesh> m_Meshes;

    // note: nothing references the static mesh renderer (or any kind of renderer) directly
    Core::Containers::Uniform::AnnotationSortedArray<int, Components::StaticMeshRenderer *> m_StaticMeshRenderers;

public:
    inline Core::Logging::Logger *GetLogger()
    {
        return &m_Logger;
    }
    void PopulateRenderCommands(Runtime::RenderContext *context);
};

} // namespace Engine::Extension::OrcaRendererModule