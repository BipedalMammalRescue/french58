#include "shader.h"
#include "pch.h"

using namespace Engine;
using namespace Extension::RendererModule;

// using SDL3 with vulkan, this function now loads a binary format SPIR-V code and uploads that to renderer service
static void *DeserializeVertexShader(Core::DependencyInjection::RuntimeServices *services,
                                     Core::AssetManagement::ByteStream *source)
{
    // load the entire file in
    unsigned char *rawData = new unsigned char[source->Count()];
    source->FillBuffer(rawData, source->Count());

    // submit shader code to the rendering service
    Core::Rendering::RendererShader rendererID;
    if (!services->GetRenderer()->CompileShader(rawData, source->Count(), Core::Rendering::ShaderType::VERTEX_SHADER, 0,
                                                1, 0, 0, rendererID))
        return nullptr;

    delete[] rawData;

    // allocate new shader data structure and return it
    return services->GetGlobalAllocator()->New<Assets::VertexShader>({rendererID});
}

static void DisposeVertexShader(Core::DependencyInjection::RuntimeServices *services, void *data)
{
    Assets::VertexShader *shader = (Assets::VertexShader *)data;
    services->GetRenderer()->DeleteShader(shader->RendererID);
    services->GetGlobalAllocator()->Free(data);
}

SE_REFLECTION_BEGIN(Extension::RendererModule::Assets::VertexShader)
    .SE_REFLECTION_OVERRIDE_DESERIALIZER(DeserializeVertexShader)
    .SE_REFLECTION_DELETE_SERIALIZER()
    .SE_REFLECTION_OVERRIDE_DISPOSER(DisposeVertexShader)
    .SE_REFLECTION_END

    static void *DeserializeFragmentShader(Core::DependencyInjection::ServiceProvider *services,
                                           Core::AssetManagement::ByteStream *source)
{
    // load the entire string in
    unsigned char *rawData = new unsigned char[source->Count()];
    source->FillBuffer(rawData, source->Count());

    // submit this to the rendering engine
    Core::Rendering::RendererShader rendererID;
    if (!services->GetRenderer()->CompileShader(rawData, source->Count(), Core::Rendering::ShaderType::FRAGMENT_SHADER,
                                                0, 0, 0, 0, rendererID))
        return nullptr;

    delete[] rawData;

    // allocate new shader data structure and return it
    return services->GetGlobalAllocator()->New<Assets::FragmentShader>({rendererID});
}

static void DisposeFragmentShader(Core::DependencyInjection::RuntimeServices *services, void *data)
{
    Assets::FragmentShader *shader = (Assets::FragmentShader *)data;
    services->GetRenderer()->DeleteShader(shader->RendererID);
    services->GetGlobalAllocator()->Free(data);
}

SE_REFLECTION_BEGIN(Extension::RendererModule::Assets::FragmentShader)
    .SE_REFLECTION_OVERRIDE_DESERIALIZER(DeserializeFragmentShader)
    .SE_REFLECTION_DELETE_SERIALIZER()
    .SE_REFLECTION_OVERRIDE_DISPOSER(DisposeFragmentShader)
    .SE_REFLECTION_END
