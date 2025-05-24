#pragma once

#include "shader.h"

#include <Rendering/renderer_data.h>

namespace Extension {
namespace RendererModule {

class RendererModule;

namespace Assets {

struct Material
{
    // not a clean solution but the reflection system is designed with simple layouts in mind for simplciity,
    // compelx objects like materials that require post-deserialization initialization needs to just declare the members
    // as fields like this
    Engine::Core::Rendering::RendererMaterial RendererID;

    static void Initialize(Engine::Core::DependencyInjection::RuntimeServices *services, void *asset);
    static void Dispose(Engine::Core::DependencyInjection::RuntimeServices *services, void *asset);
};

} // namespace Assets
} // namespace RendererModule
} // namespace Extension
