#include "EngineCore/Runtime/graphics_layer.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Logging/logger_service.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_vulkan.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <cstdint>
#include <vulkan/vulkan_core.h>

using namespace Engine::Core;
using namespace Engine::Core::Runtime;

static const char *LogChannels[] = {"GraphicsLayer"};

static constexpr uint32_t MaxFlight = 2;

#define CHECK_VULKAN(expression, error)                                                            \
    if (expression != VK_SUCCESS)                                                                  \
    return Crash(__FILE__, __LINE__, error)

constexpr const char *validationLayers[] = {"VK_LAYER_KHRONOS_validation"};

// vulkan's XXX properties structs all start with their name, so this can be used to compare
// basically all of them
static int CompareCStrs(const void *a, const void *b)
{
    const char *lhs = (const char *)a;
    const char *rhs = (const char *)b;
    return SDL_clamp(SDL_strcasecmp(lhs, rhs), -1, 1);
}

static VKAPI_ATTR VkBool32 VKAPI_CALL
VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
{
    Logging::Logger *logger = (Logging::Logger *)pUserData;

    const char messageTemplate[] = "Vulkan error: {}";

    switch (messageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        logger->Verbose(messageTemplate, pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        logger->Information(messageTemplate, pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        logger->Warning(messageTemplate, pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        logger->Error(messageTemplate, pCallbackData->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
        logger->Fatal(messageTemplate, pCallbackData->pMessage);
        break;
    }

    return VK_FALSE;
}

CallbackResult Engine::Core::Runtime::GraphicsLayer::InitializeSDL()
{
    // Create window
    m_Window = SDL_CreateWindow("Foobar Game", m_Configs->WindowWidth, m_Configs->WindowHeight, 0);
    if (m_Window == nullptr)
        return Crash(__FILE__, __LINE__,
                     std::string("Failed to create SDL window, error: ") + SDL_GetError());
    m_Logger.Information("Window created.");

    // initialize vulkan instance
    {
        if (m_Configs->EnableVulkanValidationLayers)
        {
            // check validation layer support
            uint32_t layerCount;
            vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

            std::unique_ptr<VkLayerProperties[]> availableLayers =
                std::make_unique<VkLayerProperties[]>(layerCount);
            vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.get());
            SDL_qsort(availableLayers.get(), layerCount, sizeof(VkLayerProperties), CompareCStrs);

            for (const char *neededLayer : validationLayers)
            {
                VkLayerProperties *found =
                    (VkLayerProperties *)SDL_bsearch(neededLayer, availableLayers.get(), layerCount,
                                                     sizeof(VkLayerProperties), CompareCStrs);

                if (found == nullptr)
                    return Crash(__FILE__, __LINE__,
                                 std::string("Required validation layer not found: ") +
                                     neededLayer);
            }
        }

        uint32_t vulkanVersion = 0;
        vkEnumerateInstanceVersion(&vulkanVersion);
        uint32_t majorVersion = VK_VERSION_MAJOR(vulkanVersion);
        uint32_t minorVersion = VK_VERSION_MINOR(vulkanVersion);

        if (vulkanVersion < VK_VERSION_1_3)
            return Crash(__FILE__, __LINE__,
                         "french58 currently doesn't support vulkan version < 1.3");

        VkApplicationInfo appInfo{
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = "Game",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "french58",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_MAKE_API_VERSION(0, majorVersion, minorVersion, 0),
        };

        uint32_t sdlExtensionCount = 0;
        const char *const *extensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);

        VkInstanceCreateInfo instanceInfo{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = m_Configs->EnableVulkanValidationLayers
                                     ? static_cast<uint32_t>(SDL_arraysize(validationLayers))
                                     : 0,
            .ppEnabledLayerNames = validationLayers,
            .enabledExtensionCount = sdlExtensionCount,
            .ppEnabledExtensionNames = extensions,
        };

        CHECK_VULKAN(vkCreateInstance(&instanceInfo, nullptr, &m_DeviceInfo.VkInstance),
                     "Failed to create vulkan instance!");
    }

    // set up validation debugging
    if (m_Configs->EnableVulkanValidationLayers)
    {
        VkDebugUtilsMessengerCreateInfoEXT createInfo{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = VulkanDebugCallback,
            .pUserData = this,
        };

        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            m_DeviceInfo.VkInstance, "vkCreateDebugUtilsMessengerEXT");
        if (func == nullptr)
        {
            m_Logger.Error("Failed to create debug messenger for Vulkan validation layers because "
                           "'vkCreateDebugUtilsMessengerEXT' can't be located.");
        }
        else if (func(m_DeviceInfo.VkInstance, &createInfo, nullptr,
                      &m_DeviceInfo.DebugMessenger) != VK_SUCCESS)
        {
            m_Logger.Error("Failed to create debug messenger for Vulkan validation layers due to "
                           "Vulkan error.");
        }
    }

    // create surface
    if (!SDL_Vulkan_CreateSurface(m_Window, m_DeviceInfo.VkInstance, nullptr,
                                  &m_DeviceInfo.Surface))
        return Crash(__FILE__, __LINE__,
                     std::string("Failed to create vulkan surface, error: ") + SDL_GetError());

    // initialize device
    {
        // get the physical device
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_DeviceInfo.VkInstance, &deviceCount, nullptr);

        if (deviceCount == 0)
            return Crash(__FILE__, __LINE__, "Failed to find GPUs with Vulkan support.");

        std::unique_ptr<VkPhysicalDevice[]> physicalDevices =
            std::make_unique<VkPhysicalDevice[]>(deviceCount);
        vkEnumeratePhysicalDevices(m_DeviceInfo.VkInstance, &deviceCount, physicalDevices.get());

        const char *requiredExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        SDL_qsort(requiredExtensions, SDL_arraysize(requiredExtensions), sizeof(const char *),
                  CompareCStrs);

        for (VkPhysicalDevice *physicalDevice = physicalDevices.get();
             physicalDevice < physicalDevices.get() + deviceCount; physicalDevice++)
        {
            // get device properties
            VkPhysicalDeviceProperties deviceProperties;
            VkPhysicalDeviceFeatures deviceFeatures;
            vkGetPhysicalDeviceProperties(*physicalDevice, &deviceProperties);
            vkGetPhysicalDeviceFeatures(*physicalDevice, &deviceFeatures);

            if (!deviceFeatures.samplerAnisotropy)
                continue;

            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(*physicalDevice, &queueFamilyCount, nullptr);

            std::unique_ptr<VkQueueFamilyProperties[]> queueFamilies =
                std::make_unique<VkQueueFamilyProperties[]>(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(*physicalDevice, &queueFamilyCount,
                                                     queueFamilies.get());

            // check queue availability
            std::optional<uint32_t> graphicsQueue;
            std::optional<uint32_t> presentationQueue;
            for (VkQueueFamilyProperties *queueFamily = queueFamilies.get();
                 queueFamily < queueFamilies.get() + queueFamilyCount; queueFamily++)
            {
                uint32_t currentIndex = queueFamily - queueFamilies.get();

                if (queueFamily->queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    graphicsQueue = currentIndex;
                }

                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(*physicalDevice, currentIndex,
                                                     m_DeviceInfo.Surface, &presentSupport);
                if (presentSupport)
                {
                    presentationQueue = currentIndex;
                }

                if (graphicsQueue.has_value() && presentationQueue.has_value())
                    break;
            }

            // break early if the device doesn't support
            if (!graphicsQueue.has_value() || !presentationQueue.has_value())
                continue;

            uint32_t extensionCount;
            vkEnumerateDeviceExtensionProperties(*physicalDevice, nullptr, &extensionCount,
                                                 nullptr);

            std::unique_ptr<VkExtensionProperties[]> availableExtensions =
                std::make_unique<VkExtensionProperties[]>(extensionCount);
            vkEnumerateDeviceExtensionProperties(*physicalDevice, nullptr, &extensionCount,
                                                 availableExtensions.get());

            // check extensions
            bool allExtensionsSupported = true;
            for (const char *requiredExtension : requiredExtensions)
            {
                if (nullptr == SDL_bsearch(requiredExtensions, availableExtensions.get(),
                                           extensionCount, sizeof(VkExtensionProperties),
                                           CompareCStrs))
                {
                    allExtensionsSupported = false;
                    break;
                }
            }
            if (!allExtensionsSupported)
                continue;

            // to this point the device is confirmed, let's use it
            m_DeviceInfo.PhysicalDevice = *physicalDevice;
            m_DeviceInfo.GraphicsQueueIndex = graphicsQueue.value();
            m_DeviceInfo.PresentationQueueIndex = presentationQueue.value();
            m_Logger.Information("Selected physical device: {}", deviceProperties.deviceName);
            break;
        }

        if (m_DeviceInfo.PhysicalDevice == VK_NULL_HANDLE)
            return Crash(__FILE__, __LINE__,
                         "Failed to find an adaquate physical device that supports Vulkan.");

        // create the logical device
        float queuePriority = 1.0f;
        const VkDeviceQueueCreateInfo queueCreateInfos[] = {
            (VkDeviceQueueCreateInfo){
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = m_DeviceInfo.GraphicsQueueIndex,
                .queueCount = 1,
                .pQueuePriorities = &queuePriority,
            },
            (VkDeviceQueueCreateInfo){
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = m_DeviceInfo.PresentationQueueIndex,
                .queueCount = 1,
                .pQueuePriorities = &queuePriority,
            },
        };
        uint32_t queueCount =
            m_DeviceInfo.GraphicsQueueIndex == m_DeviceInfo.PresentationQueueIndex ? 1 : 2;

        static VkPhysicalDeviceFeatures deviceFeatures{
            .samplerAnisotropy = VK_TRUE,
        };

        static VkPhysicalDeviceDescriptorIndexingFeatures bindlessFeatures{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
            .pNext = nullptr,

            .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
            .shaderStorageBufferArrayNonUniformIndexing = VK_TRUE,
            .shaderStorageImageArrayNonUniformIndexing = VK_TRUE,

            .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
            .descriptorBindingStorageImageUpdateAfterBind = VK_TRUE,
            .descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE,

            .descriptorBindingPartiallyBound = VK_TRUE,
            .runtimeDescriptorArray = VK_TRUE,
        };

        static VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeature = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
            .pNext = &bindlessFeatures,
            .dynamicRendering = VK_TRUE,
        };

        VkDeviceCreateInfo deviceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,

            .pNext = &dynamicRenderingFeature,
            .queueCreateInfoCount = queueCount,
            .pQueueCreateInfos = queueCreateInfos,

            .enabledLayerCount = m_Configs->EnableVulkanValidationLayers
                                     ? static_cast<uint32_t>(SDL_arraysize(validationLayers))
                                     : 0,
            .ppEnabledLayerNames =
                m_Configs->EnableVulkanValidationLayers ? validationLayers : nullptr,

            .enabledExtensionCount = SDL_arraysize(requiredExtensions),
            .ppEnabledExtensionNames = requiredExtensions,

            .pEnabledFeatures = &deviceFeatures,
        };

        CHECK_VULKAN(vkCreateDevice(m_DeviceInfo.PhysicalDevice, &deviceCreateInfo, nullptr,
                                    &m_DeviceInfo.Device),
                     "Failed to create vulkan logical device.");

        vkGetDeviceQueue(m_DeviceInfo.Device, m_DeviceInfo.GraphicsQueueIndex, 0,
                         &m_DeviceInfo.GraphicsQueue);
        vkGetDeviceQueue(m_DeviceInfo.Device, m_DeviceInfo.PresentationQueueIndex, 0,
                         &m_DeviceInfo.PresentationQueue);
    }

    // create the swapchain
    {
        // get general capabilities
        VkSurfaceCapabilitiesKHR swapchainCapabilities{};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_DeviceInfo.PhysicalDevice, m_DeviceInfo.Surface,
                                                  &swapchainCapabilities);
        if (swapchainCapabilities.maxImageCount <= 0)
            return Crash(__FILE__, __LINE__,
                         "Selected device doesn't support any images for swapchain.");

        // get format support and choose a format
        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_DeviceInfo.PhysicalDevice, m_DeviceInfo.Surface,
                                             &formatCount, nullptr);
        if (formatCount < 0)
            return Crash(__FILE__, __LINE__,
                         "Selected device doesn't support any formats for swapchain.");
        std::unique_ptr<VkSurfaceFormatKHR[]> surfaceFormats =
            std::make_unique<VkSurfaceFormatKHR[]>(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_DeviceInfo.PhysicalDevice, m_DeviceInfo.Surface,
                                             &formatCount, surfaceFormats.get());
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

        m_Swapchain.Format = desiredSurfaceFormat.format;
        m_Swapchain.ColorSpace = desiredSurfaceFormat.colorSpace;

        // get present modes
        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_DeviceInfo.PhysicalDevice, m_DeviceInfo.Surface,
                                                  &presentModeCount, nullptr);
        if (presentModeCount < 0)
            return Crash(__FILE__, __LINE__,
                         "Selected device doesn't support any presentation modes.");
        std::unique_ptr<VkPresentModeKHR[]> presentModes =
            std::make_unique<VkPresentModeKHR[]>(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_DeviceInfo.PhysicalDevice, m_DeviceInfo.Surface,
                                                  &presentModeCount, presentModes.get());
        VkPresentModeKHR presentMode = presentModes[0];
        for (VkPresentModeKHR *mode = &presentModes[0];
             mode < presentModes.get() + presentModeCount; mode++)
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
            extent.width =
                SDL_clamp(m_Configs->WindowWidth, swapchainCapabilities.minImageExtent.width,
                          swapchainCapabilities.maxImageExtent.width);
            extent.height =
                SDL_clamp(m_Configs->WindowHeight, swapchainCapabilities.minImageExtent.height,
                          swapchainCapabilities.maxImageExtent.height);
        }

        uint32_t imageCount =
            SDL_min(swapchainCapabilities.minImageCount + 1, swapchainCapabilities.maxImageCount);

        uint32_t queueFamilyIndices[] = {
            m_DeviceInfo.GraphicsQueueIndex,
            m_DeviceInfo.PresentationQueueIndex,
        };

        VkSwapchainCreateInfoKHR swapchainCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = m_DeviceInfo.Surface,
            .minImageCount = imageCount,
            .imageFormat = desiredSurfaceFormat.format,
            .imageColorSpace = desiredSurfaceFormat.colorSpace,
            .imageExtent = extent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode =
                m_DeviceInfo.GraphicsQueueIndex != m_DeviceInfo.PresentationQueueIndex
                    ? VK_SHARING_MODE_CONCURRENT
                    : VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount =
                m_DeviceInfo.GraphicsQueueIndex != m_DeviceInfo.PresentationQueueIndex ? (uint32_t)2
                                                                                       : 0,
            .pQueueFamilyIndices = queueFamilyIndices,
            .preTransform = swapchainCapabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = presentMode,
            .clipped = VK_TRUE,
            .oldSwapchain = VK_NULL_HANDLE,
        };

        CHECK_VULKAN(vkCreateSwapchainKHR(m_DeviceInfo.Device, &swapchainCreateInfo, nullptr,
                                          &m_Swapchain.Swapchain),
                     "Failed to create Vulkan swapchain.");

        vkGetSwapchainImagesKHR(m_DeviceInfo.Device, m_Swapchain.Swapchain, &imageCount, nullptr);
        m_Swapchain.Images.resize(imageCount);
        vkGetSwapchainImagesKHR(m_DeviceInfo.Device, m_Swapchain.Swapchain, &imageCount,
                                m_Swapchain.Images.data());

        // frame flights
        m_SwapchainViews.resize(m_Swapchain.Images.size());
        for (size_t i = 0; i < m_Swapchain.Images.size(); i++)
        {
            // create a new image view
            VkImageView newView = VK_NULL_HANDLE;
            VkImageViewCreateInfo imageViewCreateInfo{
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = m_Swapchain.Images[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = m_Swapchain.Format,
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
            if (vkCreateImageView(m_DeviceInfo.Device, &imageViewCreateInfo, nullptr, &newView) !=
                VK_SUCCESS)
                throw std::runtime_error("failed to create image views!");

            // create a new semaphore
            VkSemaphore newSemaphore = VK_NULL_HANDLE;
            VkSemaphoreCreateInfo semaphoreInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
            if (vkCreateSemaphore(m_DeviceInfo.Device, &semaphoreInfo, nullptr, &newSemaphore) !=
                VK_SUCCESS)
                throw std::runtime_error("failed to create image semaphore!");

            // register the new swapchain view resources
            m_SwapchainViews[i] = {
                .Image = m_Swapchain.Images[i],
                .View = newView,
                .RenderFinishSemaphore = newSemaphore,
            };
        }
    }

    // create the prime command pool
    {
        VkCommandPoolCreateInfo cmdPoolInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = m_DeviceInfo.GraphicsQueueIndex,
        };
        CHECK_VULKAN(vkCreateCommandPool(m_DeviceInfo.Device, &cmdPoolInfo, nullptr,
                                         &m_RenderResources.CommandPoolPrime),
                     "Failed to create Vulkan command pool prime.");
    }

    // create the global descriptor pool
    {
        VkDescriptorPoolSize poolSizes[] = {
            {
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = MaxFlight,
            },
            {
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = MaxFlight,
            },
        };
        VkDescriptorPoolCreateInfo descPoolInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = MaxFlight,
            .poolSizeCount = SDL_arraysize(poolSizes),
            .pPoolSizes = poolSizes,
        };
        CHECK_VULKAN(vkCreateDescriptorPool(m_DeviceInfo.Device, *descPoolInfo, nullptr,
                                            m_RenderResources.GlobalDescriptorPool),
                     "Failed to create descriptor pool.");
    }

    // flights
    {
        for (uint32_t i = 0; i < MaxFlight; i++)
        {
            VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
            VkCommandBufferAllocateInfo allocInfo{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = m_RenderResources.CommandPoolPrime,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1,
            };

            CHECK_VULKAN(vkAllocateCommandBuffers(m_Device.GetDevice(), &allocInfo, &cmdBuffer),
                         "Failed to create command buffer for ")
        }
    }

    return CallbackSuccess();
}

GraphicsLayer::GraphicsLayer(const Configuration::ConfigurationProvider *configs,
                             Logging::LoggerService *loggerService)
    : m_Configs(configs), m_Logger(loggerService->CreateLogger("GraphicsLayer"))
{
}