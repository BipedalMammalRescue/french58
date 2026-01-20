#pragma once

#include "EngineCore/Configuration/configuration_provider.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Runtime/crash_dump.h"
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Engine::Core {
namespace Rendering {
class RenderThread;
}
namespace Runtime {
class GraphicsLayer;
}
} // namespace Engine::Core

namespace Engine::Core::Rendering::Resources {

struct SwapchainViewResources
{
    VkImage Image;
    VkImageView View;
    VkSemaphore RenderFinishSemaphore;
};

class Swapchain
{
private:
    friend class Engine::Core::Rendering::RenderThread;
    friend class Engine::Core::Runtime::GraphicsLayer;

    VkSwapchainKHR m_Swapchain;

    VkExtent2D m_Dimensions;
    VkFormat m_Format;
    VkColorSpaceKHR m_ColorSpace;
    std::vector<VkImage> m_Images;

    std::vector<SwapchainViewResources> m_ImageViewResources;

    Runtime::CallbackResult Initialize(Logging::Logger *logger,
                                       const Configuration::ConfigurationProvider *configs,
                                       VkDevice device, VkPhysicalDevice physicalDevice,
                                       VkSurfaceKHR surface, uint32_t graphicsQueueIndex,
                                       uint32_t presentQueueIndex);

    void Dispose(VkDevice device)
    {
        for (SwapchainViewResources res : m_ImageViewResources)
        {
            vkDestroyImageView(device, res.View, nullptr);
            vkDestroySemaphore(device, res.RenderFinishSemaphore, nullptr);
        }

        vkDestroySwapchainKHR(device, m_Swapchain, nullptr);
    }
};

} // namespace Engine::Core::Rendering::Resources