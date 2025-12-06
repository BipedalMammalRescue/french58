#pragma once

#include "OrcaRendererModule/Runtime/renderer_resource.h"
#include <stddef.h>

namespace Engine::Extension::OrcaRendererModule::Assets {

// Materials is an explicit instantiation of a shader. It provides a page in the resource database.
// TODO: the implementation of this class is not correct.

struct Material
{
    // material points to a shader by reference
    size_t ShaderRef;
    size_t ResourceCount;

    Runtime::NamedRendererResource *GetResources()
    {
        return (Runtime::NamedRendererResource *)(this + 1);
    }
};

} // namespace Engine::Extension::OrcaRendererModule::Assets