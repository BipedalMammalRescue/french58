#pragma once

#include "EngineCore/Pipeline/hash_id.h"
#include "OrcaRendererModule/Runtime/renderer_resource.h"

namespace Engine::Extension::OrcaRendererModule::Assets {

// Materials is an explicit instantiation of a shader. It provides a page in the resource database.
struct Material 
{
    // material points to a shader by reference
    Core::Pipeline::HashId ShaderName;

    // A series of named resources (material is one of the root sources of renderer resource)
    Runtime::RendererResourceCollection Resources;
};

}