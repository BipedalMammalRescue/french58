#pragma once

#include <vulkan/vulkan_core.h>
namespace Engine::Core::Rendering {

struct RenderTarget
{
    VkFormat Format;
    VkImage Image;
    VkDeviceMemory Memory;
    VkImageView View;
};

} // namespace Engine::Core::Rendering