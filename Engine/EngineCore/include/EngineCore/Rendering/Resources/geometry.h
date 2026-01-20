#pragma once

#include <vulkan/vulkan_core.h>
namespace Engine::Core::Rendering::Resources {

class Geometry
{
private:
    VkBuffer m_VertexBuffer;
    VkBuffer m_IndexBuffer;

public:
    void Dispose();
};

} // namespace Engine::Core::Rendering::Resources