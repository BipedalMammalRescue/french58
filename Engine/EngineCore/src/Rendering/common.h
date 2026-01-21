#pragma once

#include "SDL3/SDL_stdinc.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>
namespace Engine::Core::Rendering {

#define CHECK_VULKAN(expression, error)                                                            \
    if (expression != VK_SUCCESS)                                                                  \
    return Engine::Core::Runtime::Crash(__FILE__, __LINE__, error)

inline int CompareCStrs(const void *a, const void *b)
{
    const char *lhs = (const char *)a;
    const char *rhs = (const char *)b;
    return SDL_clamp(SDL_strcasecmp(lhs, rhs), -1, 1);
}

inline uint32_t Log(VkResult vkResult)
{
    return (uint32_t)vkResult;
}

} // namespace Engine::Core::Rendering