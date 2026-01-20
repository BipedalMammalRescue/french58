#include "EngineCore/Rendering/Resources/swapchain.h"
#include "../common.h"

#include "EngineCore/Configuration/configuration_provider.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "SDL3/SDL_stdinc.h"

using namespace Engine::Core::Rendering::Resources;
using namespace Engine::Core::Rendering;

Engine::Core::Runtime::CallbackResult Swapchain::Initialize(
    Logging::Logger *logger, const Configuration::ConfigurationProvider *configs, VkDevice device,
    VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t graphicsQueueIndex,
    uint32_t presentQueueIndex)
{
    // get general capabilities
    VkSurfaceCapabilitiesKHR swapchainCapabilities{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &swapchainCapabilities);
    if (swapchainCapabilities.maxImageCount <= 0)
        return Runtime::Crash(__FILE__, __LINE__,
                              "Selected device doesn't support any images for swapchain.");

    // get format support and choose a format
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    if (formatCount < 0)
        return Runtime::Crash(__FILE__, __LINE__,
                              "Selected device doesn't support any formats for swapchain.");

    std::unique_ptr<VkSurfaceFormatKHR[]> surfaceFormats =
        std::make_unique<VkSurfaceFormatKHR[]>(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount,
                                         surfaceFormats.get());
    VkSurfaceFormatKHR desiredSurfaceFormat = surfaceFormats[0];
    for (VkSurfaceFormatKHR *format = &surfaceFormats[0];
         format < surfaceFormats.get() + formatCount; format++)
    {
        if (format->format == VK_FORMAT_B8G8R8_SRGB &&
            format->colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
        {
            desiredSurfaceFormat = *format;
        }
    }

    m_Format = desiredSurfaceFormat.format;
    m_ColorSpace = desiredSurfaceFormat.colorSpace;

    // get present modes
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
    if (presentModeCount < 0)
        return Runtime::Crash(__FILE__, __LINE__,
                              "Selected device doesn't support any presentation modes.");

    std::unique_ptr<VkPresentModeKHR[]> presentModes =
        std::make_unique<VkPresentModeKHR[]>(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount,
                                              presentModes.get());
    VkPresentModeKHR presentMode = presentModes[0];
    for (VkPresentModeKHR *mode = &presentModes[0]; mode < presentModes.get() + presentModeCount;
         mode++)
    {
        if (*mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            presentMode = *mode;
        }
    }

    VkExtent2D extent = swapchainCapabilities.currentExtent;
    if (extent.width == UINT32_MAX || extent.height == UINT32_MAX)
    {
        // at this point in time the window must still be the initial size, just use that value
        extent.width = SDL_clamp(configs->WindowWidth, swapchainCapabilities.minImageExtent.width,
                                 swapchainCapabilities.maxImageExtent.width);
        extent.height =
            SDL_clamp(configs->WindowHeight, swapchainCapabilities.minImageExtent.height,
                      swapchainCapabilities.maxImageExtent.height);
    }

    uint32_t imageCount =
        SDL_min(swapchainCapabilities.minImageCount + 1, swapchainCapabilities.maxImageCount);

    uint32_t queueFamilyIndices[] = {
        graphicsQueueIndex,
        presentQueueIndex,
    };

    VkSwapchainCreateInfoKHR swapchainCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = imageCount,
        .imageFormat = desiredSurfaceFormat.format,
        .imageColorSpace = desiredSurfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = graphicsQueueIndex != presentQueueIndex ? VK_SHARING_MODE_CONCURRENT
                                                                    : VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = graphicsQueueIndex != presentQueueIndex ? (uint32_t)2 : 0,
        .pQueueFamilyIndices = queueFamilyIndices,
        .preTransform = swapchainCapabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };

    CHECK_VULKAN(vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &m_Swapchain),
                 "Failed to create Vulkan swapchain.");

    m_Dimensions = extent;

    vkGetSwapchainImagesKHR(device, m_Swapchain, &imageCount, nullptr);
    m_Images.resize(imageCount);
    vkGetSwapchainImagesKHR(device, m_Swapchain, &imageCount, m_Images.data());

    // frame flights
    m_ImageViewResources.resize(m_Images.size());
    for (size_t i = 0; i < m_Images.size(); i++)
    {
        // create a new image view
        VkImageView newView = VK_NULL_HANDLE;
        VkImageViewCreateInfo imageViewCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = m_Images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = m_Format,
            .components =
                {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                },
            .subresourceRange =
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
        };
        CHECK_VULKAN(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &newView),
                     "Failed to create image views.");

        // create a new semaphore
        VkSemaphore newSemaphore = VK_NULL_HANDLE;
        VkSemaphoreCreateInfo semaphoreInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        CHECK_VULKAN(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &newSemaphore),
                     "Failed to create image semaphore!");

        // register the new swapchain view resources
        m_ImageViewResources[i] = {
            .Image = m_Images[i],
            .View = newView,
            .RenderFinishSemaphore = newSemaphore,
        };
    }

    return Runtime::CallbackSuccess();
}
