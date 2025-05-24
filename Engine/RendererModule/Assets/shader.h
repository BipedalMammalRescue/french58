#pragma once

#include <AssetManagement/asset_manager.h>
#include <DependencyInjection/buildtime_services.h>
#include <DependencyInjection/runtime_services.h>
#include <Pipeline/Scripting/variant.h>
#include <Pipeline/asset_pipeline.h>
#include <Rendering/renderer_data.h>
#include <Rendering/renderer_service.h>

namespace Engine::Extension::RendererModule::Assets {

class VertexShader
{
    static Core::Pipeline::AssetDefinition GetDefinition();

    static void Build(const Core::Pipeline::Scripting::Variant *fieldv, size_t fieldc,
                      Core::DependencyInjection::BuildtimeServies *services, std::ostream &output);

    static Core::AssetManagement::LoadedAsset Load(const unsigned char *inputDataV, const size_t inputDataC, const uint64_t id,
                                                   Core::DependencyInjection::RuntimeServices *services);

    static void Dispose(Core::AssetManagement::LoadedAsset asset, const uint64_t id,
                        Core::DependencyInjection::RuntimeServices *services);
};

struct FragmentShader
{
    Engine::Core::Rendering::RendererShader RendererID;
};

} // namespace Engine::Extension::RendererModule::Assets