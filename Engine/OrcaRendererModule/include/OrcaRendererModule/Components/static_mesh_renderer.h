#pragma once

#include "EngineCore/Pipeline/hash_id.h"

namespace Engine::Extension::OrcaRendererModule::Components {

// static meshes means the engine only supplies MVP on top of the encoded material and mesh
// does animation require the engine (code) to inject a special piece of infomration?
struct StaticMeshRenderer
{
    int Entity;
    Core::Pipeline::HashId Mesh;
    Core::Pipeline::HashId Material;
};

}