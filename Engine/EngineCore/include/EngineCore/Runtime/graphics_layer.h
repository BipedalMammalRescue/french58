#pragma once

#include "EngineCore/Configuration/configuration_provider.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Runtime/crash_dump.h"

#include <SDL3/SDL_video.h>
#include <cstdint>
#include <glm/fwd.hpp>
#include <vulkan/vulkan_core.h>

struct SDL_Window;

namespace Engine::Core::Logging {
class LoggerService;
}

namespace Engine::Core::Runtime {

class GameLoop;

// Contains states and accesses to graphics related concepts, managed by the game loop.
class GraphicsLayer
{
    // injected
private:
    const Configuration::ConfigurationProvider *m_Configs = nullptr;

    // initialized
private:
    SDL_Window *m_Window = nullptr;
    Logging::Logger m_Logger;

    // need to be initialized with vulkan
private:
    struct
    {
        VkSurfaceKHR Surface = VK_NULL_HANDLE;

        VkInstance VkInstance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT DebugMessenger = VK_NULL_HANDLE;

        VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
        VkDevice Device = VK_NULL_HANDLE;

        uint32_t GraphicsQueueIndex = UINT32_MAX;
        VkQueue GraphicsQueue = VK_NULL_HANDLE;

        uint32_t PresentationQueueIndex = UINT32_MAX;
        VkQueue PresentationQueue = VK_NULL_HANDLE;
    } m_DeviceInfo;

    struct SwapchainViewResources
    {
        VkImage Image;
        VkImageView View;
        VkSemaphore RenderFinishSemaphore;
    };
    std::vector<SwapchainViewResources> m_SwapchainViews;

    struct
    {
        VkSwapchainKHR Swapchain = VK_NULL_HANDLE;
        VkFormat Format;
        VkColorSpaceKHR ColorSpace;
        std::vector<VkImage> Images;
    } m_Swapchain = {};

    struct
    {
        VkCommandPool CommandPoolPrime = VK_NULL_HANDLE;
        VkDescriptorPool GlobalDescriptorPool = VK_NULL_HANDLE;
        VkDescriptorSetLayout GlobalDescriptorLayout = VK_NULL_HANDLE;
        VkDescriptorSet GlobalDescriptorSet = VK_NULL_HANDLE;
        VkPipelineLayout GlobalPipelineLayout = VK_NULL_HANDLE;
    } m_RenderResources;

    // the initial set of rendering related resources
private:
    // for game loop to directly control graphics behavior
private:
    friend class GameLoop;
    CallbackResult InitializeSDL();

    GraphicsLayer(const Configuration::ConfigurationProvider *configs,
                  Logging::LoggerService *loggerService);

public:
    ~GraphicsLayer();

    inline SDL_Window *GetWindow() const
    {
        return m_Window;
    }
};

} // namespace Engine::Core::Runtime