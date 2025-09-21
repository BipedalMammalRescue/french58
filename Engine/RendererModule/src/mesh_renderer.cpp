#include "RendererModule/Components/mesh_renderer.h"
#include "EngineCore/Pipeline/hash_id.h"
#include "EngineCore/Pipeline/variant.h"
#include "RendererModule/renderer_module.h"
#include <md5.h>

using namespace Engine::Extension::RendererModule;

bool Components::CompileMeshRenderer(Core::Pipeline::RawComponent input, std::ostream* output)
{
    // verify the structure is good
    if (input.FieldC != 2
        || input.FieldV[0].Payload->Type != Core::Pipeline::VariantType::Path 
        || input.FieldV[1].Payload->Type != Core::Pipeline::VariantType::Path)
        return false;

    // write out the two asset references to the output
    std::array<unsigned char, 16>* materialPath = nullptr;
    std::array<unsigned char, 16>* meshPath = nullptr;
    for (int i = 0; i < 2; i++) 
    {
        if (input.FieldV[i].Name == md5::compute("Material")) 
        {
            materialPath = &input.FieldV[i].Payload->Data.Path;
        }
        if (input.FieldV[i].Name == md5::compute("Mesh")) 
        {
            meshPath = &input.FieldV[i].Payload->Data.Path;
        }
    }

    if (materialPath == nullptr || meshPath == nullptr)
        return false;

    output->write((char*)materialPath->data(), 16);
    output->write((char*)meshPath->data(), 16);
    return true;
}

void Components::LoadMeshRenderer(size_t count, std::istream* input, Core::Runtime::ServiceTable* services, void* moduleState)
{
    ModuleState* state = static_cast<ModuleState*>(moduleState);
    state->MeshRendererComponents.reserve(state->MeshRendererComponents.size() + count);

    for (size_t i = 0; i < count; i++) 
    {
        Core::Pipeline::HashId material;
        Core::Pipeline::HashId mesh;

        input->read((char*)material.Hash.data(), 16);
        input->read((char*)mesh.Hash.data(), 16);

        state->MeshRendererComponents.push_back({ material, mesh });
    }
}