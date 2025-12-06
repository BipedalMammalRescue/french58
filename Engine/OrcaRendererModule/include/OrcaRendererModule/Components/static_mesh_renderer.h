#pragma once

#include "OrcaRendererModule/Runtime/renderer_resource.h"
#include <cstddef>

namespace Engine::Extension::OrcaRendererModule::Components {

struct StaticMeshRenderer
{
    int Entity;
    size_t MaterialRef;
    size_t RenderGraphRef;
    size_t MeshRef;

    Runtime::NamedRendererResource Mvp;
};

} // namespace Engine::Extension::OrcaRendererModule::Components