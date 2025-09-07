#pragma once

#include <EngineCore/AssetManagement/asset_manager.h>
#include <EngineCore/DependencyInjection/buildtime_services.h>
#include <EngineCore/Pipeline/Scripting/variant.h>
#include <EngineCore/Pipeline/asset_pipeline.h>
#include <EngineCore/Rendering/renderer_data.h>
#include <EngineCore/Rendering/renderer_service.h>
#include <EngineCore/Rendering/renderer_data.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace Engine::Extension::RendererModule::Assets {

struct Vertex
{
    glm::vec3 position{};
    glm::vec3 normal{};
    glm::vec2 uv{};
};

// Mesh = vertex array + index buffer, vertex array = vertex buffer + layout
class Mesh
{
  public:
    static Core::Pipeline::AssetDefinition GetDefinition();

    static bool Build(const Core::Pipeline::Scripting::Variant *fieldv, size_t fieldc,
                      Core::DependencyInjection::BuildtimeServies *services, std::ostream *output);
};

}