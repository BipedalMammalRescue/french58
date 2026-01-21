#pragma once

#include "EngineCore/Logging/logger.h"
#include "EngineCore/Runtime/crash_dump.h"

#include "EngineCore/Rendering/Lib/vk_queue_selector.h"

#include "SDL3/SDL_video.h"
#include "SDL3/SDL_vulkan.h"
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

class Device
{
private:
    friend class Engine::Core::Rendering::RenderThread;
    friend class Engine::Core::Runtime::GraphicsLayer;

    uint32_t m_Version = VK_VERSION_1_0;
    VkInstance m_Instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

    VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;

    VkDevice m_LogicalDevice = VK_NULL_HANDLE;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;

    VqsQueueSelection m_GraphicsQueueSelection = {};
    VqsQueueSelection m_TransferQueueSelection = {};

    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
    VkQueue m_PresentQueue = VK_NULL_HANDLE;
    VkQueue m_TransferQueue = VK_NULL_HANDLE;

    Runtime::CallbackResult Initialize(Engine::Core::Logging::Logger *logger,
                                       bool enableValidationLayers, SDL_Window *window);

    inline void Dispose()
    {
        vkDestroyDevice(m_LogicalDevice, nullptr);
        vkDestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
        SDL_Vulkan_DestroySurface(m_Instance, m_Surface, nullptr);
        vkDestroyInstance(m_Instance, nullptr);
    }
};

} // namespace Engine::Core::Rendering::Resources