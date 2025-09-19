#include "RendererModule/renderer_module.h"

#include "EngineCore/Pipeline/module_definition.h"
#include "EngineCore/Runtime/renderer_service.h"
#include "EngineCore/Runtime/runtime_services.h"
#include "RendererModule/Assets/mesh.h"
#include "md5.h"
#include <memory>

using namespace Engine;
using namespace Engine::Extension::RendererModule;

// TODO: solution using new operator isn't very clean
static void LoadMesh(Core::AssetManagement::AssetHeader* header, Core::Runtime::RuntimeServices* services, std::istream* input, void* output) 
{
    unsigned int vertexCount = 0;
    unsigned int indexCount = 0;

    // fill vertices
    input->read((char*)&vertexCount, sizeof(vertexCount));
    std::unique_ptr<Assets::Vertex[]> vertices = std::make_unique<Assets::Vertex[]>(vertexCount);
    input->read((char*)vertices.get(), vertexCount * sizeof(Assets::Vertex));

    // fill indices
    input->read((char*)&indexCount, sizeof(indexCount));
    std::unique_ptr<unsigned int[]> indices = std::make_unique<unsigned int[]>(indexCount);
    input->read((char*)indices.get(), indexCount * sizeof(int));

    // prepare output
    Core::Rendering::RendererMesh* outAsset = (Core::Rendering::RendererMesh*)output;

    // register mesh
    services->RendererService->RegisterMesh(
        { vertices.get(), vertexCount * (unsigned int) sizeof(Assets::Vertex) }, 
        { indices.get(), indexCount }, 
        *outAsset);
}

static void UnloadMesh(Core::Runtime::RuntimeServices* services, void* mesh) 
{
    Core::Rendering::RendererMesh* renderMesh = (Core::Rendering::RendererMesh*)mesh;
    services->RendererService->DeleteMesh(*renderMesh);
}

Engine::Core::Pipeline::ModuleDefinition Engine::Extension::RendererModule::GetModuleDefinition()
{
    static const Core::Pipeline::AssetDefinition Assets[]
    {
        {
            md5::compute("Mesh"),
            LoadMesh,
            UnloadMesh
        }
    };

    return Core::Pipeline::ModuleDefinition 
    {
        md5::compute("RendererModule"),
        Assets,
        1
    };
}
