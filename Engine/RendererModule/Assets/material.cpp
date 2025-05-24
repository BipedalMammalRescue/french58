#include "material.h"

#include <DependencyInjection/service_provider.h>

using namespace Engine;
using namespace Extension::RendererModule;

// TODO: is it a good idea to hard code vertex layout?
static Core::Rendering::VertexAttribute s_VertexLayoutHardCode[] = {
    {Core::Rendering::GpuDataType::FLOAT, 3},
    {Core::Rendering::GpuDataType::FLOAT, 3}, // normal
    {Core::Rendering::GpuDataType::FLOAT, 2}  // uv
    // TODO: add the material triple
};

constexpr size_t s_VertexLayoutLength = sizeof(s_VertexLayoutHardCode) / sizeof(Core::Rendering::VertexAttribute);

// TODO: this information is coupled into the vertex structure; the renderer module needs to be redesigned to enforce a
// pipeline structure instead of defining a language for pipeline structure
void Assets::Material::Initialize(Core::DependencyInjection::RuntimeServices *services, void *asset)
{
    Assets::Material *material = (Assets::Material *)asset;
    if (!services->GetRenderer()->CreateMaterial(material->VertShader.Data->RendererID,
                                                 material->FragShader.Data->RendererID, s_VertexLayoutHardCode,
                                                 s_VertexLayoutLength, material->RendererID))
    {
        SE_THROW_GRAPHICS_EXCEPTION;
    }
}

void Assets::Material::Dispose(Core::DependencyInjection::RuntimeServices *services, void *asset)
{
    Assets::Material *material = (Assets::Material *)asset;
    services->GetRenderer()->DeleteMaterial(material->RendererID);
}

// SE_REFLECTION_BEGIN(Extension::RendererModule::Assets::Material)
//     .SE_REFLECTION_ADD_REFERENCE(VertShader)
//     .SE_REFLECTION_ADD_REFERENCE(FragShader)
//     .SE_REFLECTION_DELETE_SERIALIZER()
//     .SE_REFLECTION_OVERRIDE_INITIALIZER(Assets::Material::Initialize)
//     .SE_REFLECTION_OVERRIDE_DISPOSER(Assets::Material::Dispose)
//     .SE_REFLECTION_END
