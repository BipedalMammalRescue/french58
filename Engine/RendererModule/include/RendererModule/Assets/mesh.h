#pragma once

#include <EngineCore/AssetManagement/asset_manager.h>
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

}