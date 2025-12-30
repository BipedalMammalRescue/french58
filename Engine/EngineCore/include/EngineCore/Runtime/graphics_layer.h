#pragma once

#include "EngineCore/Configuration/configuration_provider.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Rendering/render_target.h"
#include "EngineCore/Runtime/crash_dump.h"

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
    static constexpr uint32_t MaxFlight = 2;

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
        VkExtent2D Dimensions;
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

        VkDescriptorSetLayout PerFlightDescLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout UboLayout = VK_NULL_HANDLE;

        VkPipelineLayout GlobalPipelineLayout = VK_NULL_HANDLE;
    } m_RenderResources;

    struct
    {
        VkCommandBuffer CommandBuffer;
        VkSemaphore ImageAvailableSemaphore;
        VkFence InFlightFence;

        VkDescriptorPool DescriptorPool;
        VkDescriptorSet DescriptorSet;
    } m_CommandBuffers[MaxFlight];

    // the initial set of render targets
private:
    Rendering::RenderTarget m_RenderTargets[2];
    uint32_t m_OpqaueColorTargetId;
    uint32_t m_OpaqueDepthBufferId;

    Rendering::RenderTarget CreateRenderTarget(Rendering::RenderTargetUsage usage,
                                               Rendering::RenderTargetSetting settings);

private:
    std::vector<VkPipeline> m_GraphicsPipelines;

public:
    // TODO: there's a good point that this part of the graphics layer should be open to client
    // code. maybe expose the configuration with a filled-in default?
    // TODO: this is rather moronic, there has to a point to reuse the vertex and fragment shader
    // modules? Also there's quite a bit of setting items missing from the fixed functions, most
    // notably the fucking vertex buffer?? The only configurable element in here is the uniform
    // buffer's size.
    uint32_t CompileShaderDefault(void *vertexCode, size_t vertShaderLength, void *fragmentCode,
                                  size_t fragShaderLength, Rendering::ColorFormat *colorAttachments,
                                  uint32_t colorAttachmentCount,
                                  Rendering::DepthPrecision *depthPrecision);

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