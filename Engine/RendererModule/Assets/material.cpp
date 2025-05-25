#include "material.h"

#include <DependencyInjection/runtime_services.h>
#include <Memory/in_place_read_stream.h>
#include <ErrorHandling/exceptions.h>

using namespace Engine;
using namespace Engine::Extension::RendererModule;

// TODO: needs to change once multi-stream meshes are introduced
static Core::Rendering::VertexAttribute s_VertexLayoutHardCode[] = {
    {Core::Rendering::GpuDataType::FLOAT, 3},
    {Core::Rendering::GpuDataType::FLOAT, 3}, // normal
    {Core::Rendering::GpuDataType::FLOAT, 2}  // uv
    // TODO: add the material triple
};

constexpr size_t s_VertexLayoutLength = sizeof(s_VertexLayoutHardCode) / sizeof(Core::Rendering::VertexAttribute);

Core::Pipeline::AssetDefinition Assets::Material::GetDefinition() 
{
    static const char name[] = "Engine::Extension::Assets::Material";
    static const char fragShaderPropName[] = "Fragment Shader";
    static const char vertShaderPropName[] = "Vertex Shader";
    
    static const Core::Pipeline::Scripting::NamedProperty properties[] = {
        {
            fragShaderPropName,
            Core::Pipeline::Scripting::DataType::PATH
        },
        {
            vertShaderPropName,
            Core::Pipeline::Scripting::DataType::PATH
        }
    };
    
    static const Core::Pipeline::AssetDefinition def = 
    {
        name,
        properties,
        sizeof(properties) / sizeof(Core::Pipeline::Scripting::NamedProperty)
    };
    
    return def;
}

bool Build(const Core::Pipeline::Scripting::Variant *fieldv, size_t fieldc,
                      Core::DependencyInjection::BuildtimeServies *services, std::ostream &output)
{
    // this is a high-level asset, just write two integers into the output
    uint64_t fragShaderPath = fieldv[0].Data.Path;
    uint64_t vertShaderPath = fieldv[1].Data.Path;
    output.write(((const char*)&fragShaderPath), sizeof(uint64_t));
    output.write(((const char*)&vertShaderPath), sizeof(uint64_t));
    
    return true;
}

Core::AssetManagement::LoadedAsset Assets::Material::Load(const unsigned char *inputDataV, const size_t inputDataC, const uint64_t id,
                                                          Core::DependencyInjection::RuntimeServices *services) 
{
    Utils::Memory::InPlaceReadStream stream(inputDataV, inputDataC);
    uint64_t fragShaderId = stream.ReadCopy<uint64_t>();
    uint64_t vertShaderId = stream.ReadCopy<uint16_t>();
    
    Core::Rendering::RendererShader* fragShader = (Core::Rendering::RendererShader*)services->GetAssetManager()->GetAsset(fragShaderId).Buffer;
    Core::Rendering::RendererShader* vertShader = (Core::Rendering::RendererShader*)services->GetAssetManager()->GetAsset(vertShaderId).Buffer;
    
    Core::AssetManagement::LoadedAsset newAsset = services->GetAssetManager()->CreateAsset(sizeof(Core::Rendering::RendererMaterial), id);
    if (!services->GetRenderer()->CreateMaterial(*vertShader, *fragShader, s_VertexLayoutHardCode, s_VertexLayoutLength, 
                                                 *((Core::Rendering::RendererMaterial*)newAsset.Buffer)))
    {
        SE_THROW_GRAPHICS_EXCEPTION;
    }
    
    return newAsset;
}

void Dispose(Core::AssetManagement::LoadedAsset asset, const uint64_t id,
                        Core::DependencyInjection::RuntimeServices *services) 
{
    services->GetRenderer()->DeleteMaterial(*((Core::Rendering::RendererMaterial*)asset.Buffer));
}