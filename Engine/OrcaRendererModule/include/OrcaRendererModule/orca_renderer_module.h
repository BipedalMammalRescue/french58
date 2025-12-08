#pragma once

#include "EngineCore/Containers/Uniform/sorted_array.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "OrcaRendererModule/Assets/material.h"
#include "OrcaRendererModule/Assets/mesh.h"
#include "OrcaRendererModule/Assets/render_graph.h"
#include "OrcaRendererModule/Assets/shader.h"
#include "OrcaRendererModule/Components/static_mesh_renderer.h"
#include "OrcaRendererModule/Runtime/reference_list.h"
#include "OrcaRendererModule/Runtime/renderer.h"

namespace Engine::Extension::OrcaRendererModule {

namespace Runtime {
class RenderContext;
}

// NOTE: we use liberally fixed-size arrays for things that don't have a reason to be long

struct ModuleState
{
    Core::Logging::Logger Logger;

    Runtime::Renderer Renderer;

    Runtime::ReferenceList<Assets::RenderGraph> RenderGraphs;
    Runtime::ReferenceList<Assets::Shader> Shaders;
    Runtime::ReferenceList<Assets::Material> Materials;

    Runtime::ReferenceList<Assets::Mesh> Meshes;

    // Textures are Managed by the renderer; this is just a dictionary of file path -> renderer
    // resource id mapping.
    Core::Containers::Uniform::AnnotationSortedArray<Core::Pipeline::HashId, uint32_t> Textures;

    // note: nothing references the static mesh renderer (or any kind of renderer) directly
    Core::Containers::Uniform::AnnotationSortedArray<int, Components::StaticMeshRenderer *>
        StaticMeshRenderers;

    const Assets::Shader *FindShader(Core::Pipeline::HashId name) const;
    const Assets::Material *FindMaterial(Core::Pipeline::HashId name) const;
    const Assets::RenderGraph *FindGraph(Core::Pipeline::HashId name) const;

    void PopulateRenderCommands(Runtime::RenderContext *context);
};

} // namespace Engine::Extension::OrcaRendererModule