#pragma once

#include "EngineCore/Pipeline/component_definition.h"
#include "RendererModule/Assets/material.h"
#include "RendererModule/Assets/mesh.h"

namespace Engine::Extension::RendererModule::Components {

bool CompileMeshRenderer(Core::Pipeline::RawComponent input, std::ostream* output);
Core::Runtime::CallbackResult LoadMeshRenderer(size_t count, std::istream* input, Core::Runtime::ServiceTable* services, void* moduleState);

struct MeshRenderer
{
    int Entity;
    Assets::GpuMesh Mesh;
    Assets::Material Material;
};

}