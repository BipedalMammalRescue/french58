#pragma once

#include "shader.h"

#include <Rendering/renderer_data.h>

namespace Engine::Extension::RendererModule::Assets {

class Material
{ 
  public:   
    static Core::Pipeline::AssetDefinition GetDefinition();

    static bool Build(const Core::Pipeline::Scripting::Variant *fieldv, size_t fieldc,
                      Core::DependencyInjection::BuildtimeServies *services, std::ostream &output);

    static Core::AssetManagement::LoadedAsset Load(const unsigned char *inputDataV, const size_t inputDataC, const uint64_t id,
                                                   Core::DependencyInjection::RuntimeServices *services);

    static void Dispose(Core::AssetManagement::LoadedAsset asset, const uint64_t id,
                        Core::DependencyInjection::RuntimeServices *services);
};

}