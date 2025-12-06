#pragma once

#include "OrcaRendererModule/Runtime/renderer_resource.h"
#include <cstddef>

namespace Engine::Extension::OrcaRendererModule::Components {

// static meshes means the engine only supplies MVP on top of the encoded material and mesh
// does animation require the engine (code) to inject a special piece of infomration?
struct StaticMeshRenderer
{
    int Entity;
    size_t MaterialRef;
    size_t RenderGraphRef;
    size_t MeshRef;
    Runtime::RendererResourceCollection DataCollection;
};

} // namespace Engine::Extension::OrcaRendererModule::Components