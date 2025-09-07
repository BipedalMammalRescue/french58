#include "RendererModule/Assets/shader.h"
#include <fstream>

using namespace Engine;
using namespace Engine::Extension::RendererModule;

Core::Pipeline::AssetDefinition Engine::Extension::RendererModule::Assets::VertexShader::GetDefinition()
{
    static const char name[] = "Engine::Extension::RendererModule::Assets::VertexShader";
    static const char propType[] = "Source";

    static const Core::Pipeline::Scripting::NamedProperty properties[] = {
        {propType, Core::Pipeline::Scripting::DataType::PATH}};

    static Core::Pipeline::AssetDefinition def = {
        name, properties, sizeof(properties) / sizeof(Core::Pipeline::Scripting::NamedProperty)};

    return def;
}

bool Engine::Extension::RendererModule::Assets::VertexShader::Build(
    const Core::Pipeline::Scripting::Variant *fieldv, size_t fieldc,
    Core::DependencyInjection::BuildtimeServies *services, std::ostream &output)
{
    // safety is not my concern
    const char *sourceShader = services->GetFileAccessService()->GetPath(fieldv[0].Data.Path);

    // read the file and dump it into the output stream
    std::ofstream sourceFile;
    sourceFile.open(sourceShader, std::ios::binary);
    output << sourceFile.rdbuf();
    sourceFile.close();
    return true;
}

// size_t Assets::VertexShader::MaxLoadSize(const unsigned char *inputDataV, const size_t inputDataC, const uint64_t id,
//                                  Core::DependencyInjection::RuntimeServices *services)
// {
//     return sizeof(Core::Rendering::RendererShader);
// }

// Core::AssetManagement::LoadedAsset Engine::Extension::RendererModule::Assets::VertexShader::Load(
//     const unsigned char *inputDataV, const size_t inputDataC, const uint64_t id,
//     Core::DependencyInjection::RuntimeServices *services)
// {
//     // submit shader code to the rendering service
//     Core::Rendering::RendererShader rendererID;
//     if (!services->GetRenderer()->CompileShader(inputDataV, inputDataC, Core::Rendering::ShaderType::VERTEX_SHADER, 0,
//                                                 1, 0, 0, rendererID))
//         return {nullptr, 0};

//     // allocate new shader data structure and return it
//     Core::AssetManagement::LoadedAsset newAsset = services->GetAssetManager()->CreateAsset(sizeof(rendererID), id);
//     *((Core::Rendering::RendererShader *)newAsset.Buffer) = rendererID;
//     return newAsset;
// }

// void Engine::Extension::RendererModule::Assets::VertexShader::Dispose(
//     Core::AssetManagement::LoadedAsset asset, const uint64_t id, Core::DependencyInjection::RuntimeServices *services)
// {
//     services->GetRenderer()->DeleteShader(*(Core::Rendering::RendererShader *)asset.Buffer);
// }

Core::Pipeline::AssetDefinition Engine::Extension::RendererModule::Assets::FragmentShader::GetDefinition()
{
    static const char name[] = "Engine::Extension::RendererModule::Assets::FragmentShader";
    static const char propType[] = "Source";

    static const Core::Pipeline::Scripting::NamedProperty properties[] = {
        {propType, Core::Pipeline::Scripting::DataType::PATH}};

    static Core::Pipeline::AssetDefinition def = {
        name, properties, sizeof(properties) / sizeof(Core::Pipeline::Scripting::NamedProperty)};

    return def;
}

bool Engine::Extension::RendererModule::Assets::FragmentShader::Build(
    const Core::Pipeline::Scripting::Variant *fieldv, size_t fieldc,
    Core::DependencyInjection::BuildtimeServies *services, std::ostream &output)
{
    // safety is not my concern
    const char *sourceShader = services->GetFileAccessService()->GetPath(fieldv[0].Data.Path);

    // read the file and dump it into the output stream
    std::ofstream sourceFile;
    sourceFile.open(sourceShader, std::ios::binary);
    output << sourceFile.rdbuf();
    sourceFile.close();
    return true;
}

// size_t Assets::FragmentShader::MaxLoadSize(const unsigned char *inputDataV, const size_t inputDataC, const uint64_t id,
//                                            Core::DependencyInjection::RuntimeServices *services)
// {
//     return sizeof(Core::Rendering::RendererShader);
// }

// Core::AssetManagement::LoadedAsset Engine::Extension::RendererModule::Assets::FragmentShader::Load(
//     const unsigned char *inputDataV, const size_t inputDataC, const uint64_t id,
//     Core::DependencyInjection::RuntimeServices *services)
// {
//     // submit shader code to the rendering service
//     Core::Rendering::RendererShader rendererID;
//     if (!services->GetRenderer()->CompileShader(inputDataV, inputDataC, Core::Rendering::ShaderType::FRAGMENT_SHADER, 0,
//                                                 1, 0, 0, rendererID))
//         return {nullptr, 0};

//     // allocate new shader data structure and return it
//     Core::AssetManagement::LoadedAsset newAsset = services->GetAssetManager()->CreateAsset(sizeof(rendererID), id);
//     *((Core::Rendering::RendererShader *)newAsset.Buffer) = rendererID;
//     return newAsset;
// }

// void Engine::Extension::RendererModule::Assets::FragmentShader::Dispose(
//     Core::AssetManagement::LoadedAsset asset, const uint64_t id, Core::DependencyInjection::RuntimeServices *services)
// {
//     services->GetRenderer()->DeleteShader(*(Core::Rendering::RendererShader *)asset.Buffer);
// }