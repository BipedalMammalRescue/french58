#pragma once

#include "EngineCore/Pipeline/component_definition.h"
#include "EngineCore/Pipeline/hash_id.h"

namespace Engine::Extension::RendererModule::Components {

bool CompileMeshRenderer(Core::Pipeline::RawComponent input, std::ostream* output);
void LoadMeshRenderer(size_t count, std::istream* input, Core::Runtime::ServiceTable* services, void* moduleState);

struct MeshRenderer
{
    int Entity;
    Core::Pipeline::HashId Material;
    Core::Pipeline::HashId Mesh;
};

}