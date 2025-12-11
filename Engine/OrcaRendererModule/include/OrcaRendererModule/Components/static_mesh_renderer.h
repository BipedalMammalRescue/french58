#pragma once

#include "OrcaRendererModule/Runtime/renderer_resource.h"
#include "OrcaRendererModule/utils.h"
#include "glm/ext/matrix_float4x4.hpp"
#include <cstddef>

namespace Engine::Extension::OrcaRendererModule::Components {

struct StaticMeshRenderer
{
    int Entity;

    size_t MaterialRef;
    size_t RenderGraphRef;
    size_t MeshRef;

    DoubleBuffer<glm::mat4> MvpBuffer;

    Runtime::NamedRendererResource MvpIndex;
};

} // namespace Engine::Extension::OrcaRendererModule::Components