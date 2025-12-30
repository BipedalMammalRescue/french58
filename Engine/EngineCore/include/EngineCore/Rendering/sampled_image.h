#pragma once

#include <vulkan/vulkan_core.h>
namespace Engine::Core::Rendering {

struct SampledImage
{
    VkImage Image;
    VkSampler Sampler;
};

} // namespace Engine::Core::Rendering