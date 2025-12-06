#pragma once

#include "OrcaRendererModule/Assets/shader.h"
#include "OrcaRendererModule/Runtime/reference_list.h"
#include "OrcaRendererModule/Runtime/renderer_resource.h"

namespace Engine::Extension::OrcaRendererModule::Assets {

// Materials is an explicit instantiation of a shader. It provides a page in the resource database.
// TODO: the implementation of this class is not correct.
struct Material
{
    // material points to a shader by reference
    size_t ShaderRef;

    // A series of named resources (material is one of the root sources of renderer resource)
    Runtime::RendererResourceCollection Resources;
};

} // namespace Engine::Extension::OrcaRendererModule::Assets