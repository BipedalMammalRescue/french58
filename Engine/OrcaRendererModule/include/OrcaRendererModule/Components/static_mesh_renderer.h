#pragma once

#include "EngineCore/Pipeline/hash_id.h"
namespace Engine::Extension::OrcaRendererModule {

struct StaticMeshRenderer
{
    Core::Pipeline::HashId Mesh;
    Core::Pipeline::HashId Material;
};

}