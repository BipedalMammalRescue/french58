#include "EngineCore/Runtime/graphics_layer.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Logging/logger_service.h"
#include "EngineCore/Rendering/gpu_resource.h"
#include "EngineCore/Rendering/render_target.h"
#include "EngineCore/Rendering/vertex_description.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_vulkan.h"
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float3.hpp"

#include <cmath>
#include <cstdint>
#include <memory>
#include <vulkan/vulkan_core.h>

using namespace Engine::Core;
using namespace Engine::Core::Runtime;
using namespace Engine::Core::Rendering;

static const char *LogChannels[] = {"GraphicsLayer"};

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

                if (!graphicsQueue.has_value() && queueFamily->queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    graphicsQueue = currentIndex;
                }

                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(*physicalDevice, currentIndex,
                                                     m_DeviceInfo.Surface, &presentSupport);
                if (!presentationQueue.has_value() && presentSupport)
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

            // TODO: eventually make the transfer queue a different queue
            m_DeviceInfo.TransferQueueIndex = graphicsQueue.value();
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
        uint32_t queueCount = 2;
        if (m_DeviceInfo.PresentationQueueIndex != m_DeviceInfo.GraphicsQueueIndex)
        {
            queueCount++;
        }

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
        // TODO: implement separate queue for transfers
        m_DeviceInfo.TransferQueue = m_DeviceInfo.GraphicsQueue;
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

        m_Swapchain.Dimensions = extent;

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

    // create transfer utils
    {
        VkCommandPoolCreateInfo cmdPoolInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                     VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = m_DeviceInfo.TransferQueueIndex,
        };
        CHECK_VULKAN(vkCreateCommandPool(m_DeviceInfo.Device, &cmdPoolInfo, nullptr,
                                         &m_TransferUtils.TransferCmdPool),
                     "Failed to create Vulkan transfer command pool.");

        VkCommandBufferAllocateInfo cmdBufferInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = m_TransferUtils.TransferCmdPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        CHECK_VULKAN(vkAllocateCommandBuffers(m_DeviceInfo.Device, &cmdBufferInfo,
                                              &m_TransferUtils.TransferCmdBuffer),
                     "Failed to allocate transfer command buffer.");

        VkFenceCreateInfo fenceInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
        };
        CHECK_VULKAN(
            vkCreateFence(m_DeviceInfo.Device, &fenceInfo, nullptr, &m_TransferUtils.TransferFence),
            "Failed to create transfer fence.");
    }

    // bindless setup (and pretty much all the metadata needed for pipeline layout)
    {
        static constexpr VkDescriptorPoolSize globalPoolSizes[] = {
            (VkDescriptorPoolSize){
                .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 65536,
            },
            (VkDescriptorPoolSize){
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 65536,
            },
        };
        static constexpr VkDescriptorPoolCreateInfo globalPoolInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
            .maxSets = 1,
            .poolSizeCount = SDL_arraysize(globalPoolSizes),
            .pPoolSizes = globalPoolSizes,
        };
        CHECK_VULKAN(vkCreateDescriptorPool(m_DeviceInfo.Device, &globalPoolInfo, nullptr,
                                            &m_RenderResources.GlobalDescriptorPool),
                     "Failed to allocate bindless descriptor pool.");

        static constexpr VkDescriptorSetLayoutBinding globalSetBindings[] = {
            (VkDescriptorSetLayoutBinding){
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 65536,
                .stageFlags = VK_SHADER_STAGE_ALL,
            },
            (VkDescriptorSetLayoutBinding){
                .binding = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 65536,
                .stageFlags = VK_SHADER_STAGE_ALL,
            },
            (VkDescriptorSetLayoutBinding){
                .binding = 2,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .descriptorCount = 65536,
                .stageFlags = VK_SHADER_STAGE_ALL,
            },
        };
        VkDescriptorSetLayoutCreateInfo globalSetLayoutCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
            .bindingCount = SDL_arraysize(globalSetBindings),
            .pBindings = globalSetBindings,
        };
        CHECK_VULKAN(vkCreateDescriptorSetLayout(m_DeviceInfo.Device, &globalSetLayoutCreateInfo,
                                                 nullptr,
                                                 &m_RenderResources.GlobalDescriptorLayout),
                     "Failed to create bindless descriptor set layout.");

        VkDescriptorSetAllocateInfo globalsetAllocateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = nullptr,
            .descriptorPool = m_RenderResources.GlobalDescriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &m_RenderResources.GlobalDescriptorLayout,
        };
        CHECK_VULKAN(vkAllocateDescriptorSets(m_DeviceInfo.Device, &globalsetAllocateInfo,
                                              &m_RenderResources.GlobalDescriptorSet),
                     "Failed to allocate bindless descriptor set.");

        static constexpr VkDescriptorSetLayoutBinding perFlightSetBindings[] = {
            (VkDescriptorSetLayoutBinding){
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 65536,
                .stageFlags = VK_SHADER_STAGE_ALL,
            },
        };
        static constexpr VkDescriptorSetLayoutCreateInfo perFlightSetLayoutCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
            .bindingCount = SDL_arraysize(perFlightSetBindings),
            .pBindings = perFlightSetBindings,
        };
        CHECK_VULKAN(vkCreateDescriptorSetLayout(m_DeviceInfo.Device, &perFlightSetLayoutCreateInfo,
                                                 nullptr, &m_RenderResources.PerFlightDescLayout),
                     "Failed to create render-graph related descriptor set layout.");

        static constexpr VkDescriptorSetLayoutBinding uniformBufferBindings[] = {
            (VkDescriptorSetLayoutBinding){
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_ALL,
            }};
        static constexpr VkDescriptorSetLayoutCreateInfo uniformBufferLayoutCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .bindingCount = SDL_arraysize(uniformBufferBindings),
            .pBindings = uniformBufferBindings,
        };
        CHECK_VULKAN(vkCreateDescriptorSetLayout(m_DeviceInfo.Device,
                                                 &uniformBufferLayoutCreateInfo, nullptr,
                                                 &m_RenderResources.UboLayout),
                     "Failed to create ubo descriptor set layout.");

        // now with the layouts created this is enough information to create a pipeline layout

        VkPushConstantRange pushConstant[] = {
            (VkPushConstantRange){
                .stageFlags = VK_SHADER_STAGE_ALL,
                .offset = 0,
                .size = 128,
            },
        };
        VkDescriptorSetLayout layouts[]{
            m_RenderResources.GlobalDescriptorLayout,
            m_RenderResources.PerFlightDescLayout,
            m_RenderResources.UboLayout,
        };
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = SDL_arraysize(layouts),
            .pSetLayouts = layouts,
            .pushConstantRangeCount = SDL_arraysize(layouts),
            .pPushConstantRanges = pushConstant,
        };
        CHECK_VULKAN(vkCreatePipelineLayout(m_DeviceInfo.Device, &pipelineLayoutInfo, nullptr,
                                            &m_RenderResources.GlobalPipelineLayout),
                     "Failed to create global graphics pipeline layout.");
    }

    // command buffers
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

            CHECK_VULKAN(vkAllocateCommandBuffers(m_DeviceInfo.Device, &allocInfo, &cmdBuffer),
                         "Failed to create Vulkan command buffer");

            // a set of synchronizers
            VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
            VkFence inFlightFence = VK_NULL_HANDLE;

            VkSemaphoreCreateInfo semaphoreInfo{
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            };

            VkFenceCreateInfo fenceInfo{
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .flags = VK_FENCE_CREATE_SIGNALED_BIT,
            };

            if (vkCreateSemaphore(m_DeviceInfo.Device, &semaphoreInfo, nullptr,
                                  &imageAvailableSemaphore) != VK_SUCCESS ||
                vkCreateFence(m_DeviceInfo.Device, &fenceInfo, nullptr, &inFlightFence) !=
                    VK_SUCCESS)
                return Crash(__FILE__, __LINE__, "failed to create semaphores!");

            // create the double-buffered render target accessing
            VkDescriptorPool flightPool = VK_NULL_HANDLE;
            static constexpr VkDescriptorPoolSize flightPoolSizes[] = {
                (VkDescriptorPoolSize){
                    .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 65536,
                },
            };
            static constexpr VkDescriptorPoolCreateInfo flightPoolAllocInfo = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
                .maxSets = 1,
                .poolSizeCount = SDL_arraysize(flightPoolSizes),
                .pPoolSizes = flightPoolSizes,
            };
            CHECK_VULKAN(vkCreateDescriptorPool(m_DeviceInfo.Device, &flightPoolAllocInfo, nullptr,
                                                &flightPool),
                         "Failed to allocate bindless descriptor pool.");

            VkDescriptorSet flightDescSet = VK_NULL_HANDLE;
            VkDescriptorSetAllocateInfo flightDescSetInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .pNext = nullptr,
                .descriptorPool = flightPool,
                .descriptorSetCount = 1,
                .pSetLayouts = &m_RenderResources.PerFlightDescLayout,
            };
            CHECK_VULKAN(
                vkAllocateDescriptorSets(m_DeviceInfo.Device, &flightDescSetInfo, &flightDescSet),
                "Failed to allocate double-buffered resource descriptor sets.");

            m_CommandBuffers[i] = {
                .CommandBuffer = cmdBuffer,
                .ImageAvailableSemaphore = imageAvailableSemaphore,
                .InFlightFence = inFlightFence,
                .DescriptorPool = flightPool,
                .DescriptorSet = flightDescSet,
            };
        }
    }

    // set up the critical path rendering targets (THESE ARE NOT BACKED BY MEMORY JUST YET)
    {
        m_OpqaueColorTargetId = 0;
        m_RenderTargets[0] =
            CreateRenderTarget(RenderTargetUsage::ColorTarget,
                               (RenderTargetSetting){
                                   .Image =
                                       {
                                           .ColorTarget =
                                               {
                                                   .ScaleToSwapchain = true,
                                                   .Dimensions = {.Relative = {1.0f, 1.0f}},
                                                   .ColorFormat = ColorFormat::UseSwapchain,
                                               },
                                       },
                                   .Sampler = Rendering::SamplerType::None,
                               });

        m_OpaqueDepthBufferId = 1;
        m_RenderTargets[1] =
            CreateRenderTarget(Rendering::RenderTargetUsage::DepthBuffer,
                               (RenderTargetSetting){
                                   .Image =
                                       {
                                           .ColorTarget =
                                               {
                                                   .ScaleToSwapchain = true,
                                                   .Dimensions = {.Relative = {1.0f, 1.0f}},
                                                   .ColorFormat = ColorFormat::UseSwapchain,
                                               },
                                       },
                                   .Sampler = Rendering::SamplerType::None,
                               });
    }

    return CallbackSuccess();
}

Rendering::RenderTarget GraphicsLayer::CreateRenderTarget(Rendering::RenderTargetUsage usage,
                                                          Rendering::RenderTargetSetting settings)
{
    VkFormat format;
    uint32_t width;
    uint32_t height;

    // TODO: when do I need to change the tiling setting?
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    VkImageUsageFlags gpuUsage;
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    switch (usage)
    {
    case RenderTargetUsage::ColorTarget: {
        switch (settings.Image.ColorTarget.ColorFormat)
        {
        case ColorFormat::UseSwapchain:
            format = m_Swapchain.Format;
            break;
        }

        if (settings.Image.ColorTarget.ScaleToSwapchain)
        {
            width = static_cast<uint32_t>(
                round(settings.Image.ColorTarget.Dimensions.Relative.WidthScale *
                      m_Swapchain.Dimensions.width));
            height = static_cast<uint32_t>(
                round(settings.Image.ColorTarget.Dimensions.Relative.HeightScale *
                      m_Swapchain.Dimensions.height));
        }
        else
        {
            width = settings.Image.ColorTarget.Dimensions.Absolute.Width;
            height = settings.Image.ColorTarget.Dimensions.Absolute.Height;
        }

        gpuUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    break;
    case RenderTargetUsage::DepthBuffer: {
        switch (settings.Image.DepthBuffer.Precision)
        {
        case Engine::Core::Rendering::DepthPrecision::D32:
            format = VK_FORMAT_D32_SFLOAT;
            break;
        }

        if (settings.Image.DepthBuffer.ScaleToSwapchain)
        {
            width = static_cast<uint32_t>(
                round(settings.Image.DepthBuffer.Dimensions.Relative.WidthScale *
                      m_Swapchain.Dimensions.width));
            height = static_cast<uint32_t>(
                round(settings.Image.DepthBuffer.Dimensions.Relative.HeightScale *
                      m_Swapchain.Dimensions.height));
        }
        else
        {
            width = settings.Image.DepthBuffer.Dimensions.Absolute.Width;
            height = settings.Image.DepthBuffer.Dimensions.Absolute.Height;
        }
    }
    break;
    }

    if (settings.Sampler > Rendering::SamplerType::None)
    {
        gpuUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }

    GpuImage image = CreateImage(m_DeviceInfo.Device, m_DeviceInfo.PhysicalDevice, width, height,
                                 format, tiling, gpuUsage, properties);

    return {
        .Setting = settings,
        .Image = image,
        .View = VK_NULL_HANDLE,
    };
}

GraphicsLayer::GraphicsLayer(const Configuration::ConfigurationProvider *configs,
                             Logging::LoggerService *loggerService)
    : m_Configs(configs), m_Logger(loggerService->CreateLogger("GraphicsLayer"))
{
}

class TemporaryShaderModule
{
private:
    VkDevice m_Device = VK_NULL_HANDLE;
    VkShaderModule m_Shader = VK_NULL_HANDLE;
    VkResult m_InitResult = VK_ERROR_UNKNOWN;

public:
    TemporaryShaderModule(void *code, size_t length, VkDevice device) : m_Device(device)
    {
        VkShaderModuleCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = length,
            .pCode = reinterpret_cast<const uint32_t *>(code),
        };

        m_InitResult = vkCreateShaderModule(device, &createInfo, nullptr, &m_Shader);
    }

    inline bool Success() const
    {
        return m_InitResult == VK_SUCCESS;
    }

    inline VkShaderModule Get() const
    {
        return m_Shader;
    }

    ~TemporaryShaderModule()
    {
        if (m_Device != VK_NULL_HANDLE && m_Shader != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(m_Device, m_Shader, nullptr);
        }
    }
};

static VkShaderModule CreateShaderModule(void *code, size_t length, VkDevice device)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = length;
    createInfo.pCode = reinterpret_cast<const uint32_t *>(code);

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        throw std::runtime_error("failed to create shader module!");

    return shaderModule;
}

static void TranslateVertexAttributes(const VertexDescription *vertexSetting,
                                      VkVertexInputAttributeDescription *destAttr,
                                      VkVertexInputBindingDescription *destBinding)
{
    size_t attrOffset = 0;

    for (uint32_t i = 0; i < vertexSetting->BindingCount; i++)
    {
        uint32_t stride = 0;

        for (uint32_t attrId = 0; attrId < vertexSetting->Bindings[i].AttributeCount; attrId++)
        {
            uint32_t offset = stride;
            VkFormat format = VK_FORMAT_UNDEFINED;

            switch (vertexSetting->Bindings[i].Attributes[attrId])
            {
            case VertexDataTypes::Float:
                stride += sizeof(float);
                format = VK_FORMAT_R32_SFLOAT;
                break;
            case VertexDataTypes::Vector2:
                stride += sizeof(glm::vec2);
                format = VK_FORMAT_R32G32_SFLOAT;
                break;
            case VertexDataTypes::Vector3:
                stride += sizeof(glm::vec3);
                format = VK_FORMAT_R32G32B32_SFLOAT;
                break;
            case VertexDataTypes::Vector4:
                stride += sizeof(glm::vec4);
                format = VK_FORMAT_R32G32B32A32_SFLOAT;
                break;
            }

            destAttr[attrOffset] = {
                .location = attrId,
                .binding = i,
                .format = format,
                .offset = offset,
            };

            attrOffset++;
        }

        destBinding[i] = {
            .binding = i,
            .stride = stride,

            // TODO: the engine doesn't support instanced rendering rn, will need to fix it here
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };
    }
}

uint32_t GraphicsLayer::CompileShader(void *vertexCode, size_t vertShaderLength, void *fragmentCode,
                                      size_t fragShaderLength,
                                      const Rendering::VertexDescription *vertexSetting,
                                      const Rendering::PipelineSetting *pipelineSetting,
                                      const Rendering::ColorFormat *colorAttachments,
                                      uint32_t colorAttachmentCount,
                                      const Rendering::DepthPrecision *depthPrecision)
{
    if (vertexSetting->BindingCount > MaxVertexBufferBindings)
    {
        m_Logger.Error("Shader contains {} bindings, where only {} is allowed.",
                       vertexSetting->BindingCount, MaxVertexBufferBindings);
        return UINT32_MAX;
    }

    uint32_t totalAttributeCount = 0;
    for (auto *binding = vertexSetting->Bindings;
         binding < vertexSetting->Bindings + vertexSetting->BindingCount; binding++)
    {
        totalAttributeCount += binding->AttributeCount;
    }

    if (totalAttributeCount > MaxVertexBufferAttributes)
    {
        m_Logger.Error("Shader contains {} total attributes, where only {} is allowed.",
                       totalAttributeCount, MaxVertexBufferAttributes);
    }

    VkVertexInputBindingDescription bindings[MaxVertexBufferBindings];
    VkVertexInputAttributeDescription attributes[MaxVertexBufferAttributes];
    TranslateVertexAttributes(vertexSetting, attributes, bindings);

    // create the shader modules
    TemporaryShaderModule vertShader(vertexCode, vertShaderLength, m_DeviceInfo.Device);
    TemporaryShaderModule fragShader(fragmentCode, fragShaderLength, m_DeviceInfo.Device);
    if (!vertShader.Success() || !fragShader.Success())
        return UINT32_MAX;

    // create the pipeline
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertShader.Get(),
        .pName = "main",
    };
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragShader.Get(),
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                                 VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data(),
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = vertexSetting->BindingCount,
        .pVertexBindingDescriptions = bindings,
        .vertexAttributeDescriptionCount = totalAttributeCount,
        .pVertexAttributeDescriptions = attributes,
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = nullptr,
        .scissorCount = 1,
        .pScissors = nullptr,
    };

    VkPipelineRasterizationStateCreateInfo rasterizer{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisampling{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    // TODO: there's been no knowledge how to change this yet
    VkPipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo colorBlending{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY, // TODO: wtf is this?
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
        .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f} // optional
    };

    // infer the color formats
    std::unique_ptr<VkFormat[]> formats = std::make_unique<VkFormat[]>(colorAttachmentCount);
    for (size_t i = 0; i < colorAttachmentCount; i++)
    {
        formats[i] = m_Swapchain.Format;
        switch (colorAttachments[i])
        {
        case ColorFormat::UseSwapchain:
            break;
        }
    }

    // infer the depth format
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;
    if (depthPrecision != nullptr)
    {
        switch (*depthPrecision)
        {
        case DepthPrecision::D32:
            depthFormat = VK_FORMAT_D32_SFLOAT;
            break;
        }
    }

    // this one is rather proper
    VkPipelineRenderingCreateInfo renderingCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .pNext = nullptr,
        .viewMask = 0,
        .colorAttachmentCount = colorAttachmentCount,
        .pColorAttachmentFormats = formats.get(),
        .depthAttachmentFormat = depthFormat,
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
    };

    // TODO: rn we hard-code that depth-test is always enabled with depth-write
    VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = nullptr,
        .depthTestEnable = depthPrecision ? VK_TRUE : VK_FALSE,
        .depthWriteEnable = depthPrecision ? VK_TRUE : VK_FALSE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
    };

    // create the pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,

        // this should be controlled by the engine (feature enabling)
        .pNext = &renderingCreateInfo,

        // this is managed by the engine too (engine only uses two stages)
        .stageCount = 2,
        .pStages = shaderStages,

        // this should be controlled by the client (data format)
        .pVertexInputState = &vertexInputInfo,

        // this should be managed by the engine (engine only allows triangle lists)
        .pInputAssemblyState = &inputAssembly,

        // controlled by the engine (only use dynamic state for viewport and scissor)
        .pViewportState = &viewportState,

        // should be exposed to the client, this is closely related to the desired behavior
        .pRasterizationState = &rasterizer,

        // probably controlled by the engine? multisampling feels like something that should be
        // configured together
        .pMultisampleState = &multisampling,

        // managed by the depth stencil parameters
        .pDepthStencilState = &depthStencilCreateInfo,

        // maybe controlled by the client? no need for it yet
        .pColorBlendState = &colorBlending,

        // controlled by the engine all of the below
        .pDynamicState = &dynamicState,
        .layout = m_RenderResources.GlobalPipelineLayout,
        .renderPass = VK_NULL_HANDLE,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE, // Optional
        .basePipelineIndex = -1               // Optional
    };

    VkPipeline newPipe = VK_NULL_HANDLE;
    if (vkCreateGraphicsPipelines(m_DeviceInfo.Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                                  &newPipe) != VK_SUCCESS)
        return UINT32_MAX;

    m_GraphicsPipelines.push_back(newPipe);
    return m_GraphicsPipelines.size() - 1;
}

StagingBuffer *GraphicsLayer::CreateStagingBuffer(size_t size)
{
    // TODO: allocate buffers like this from a free list
    StagingBuffer *newBuffer = new StagingBuffer;
    newBuffer->m_Size = size;

    VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    if (vkCreateBuffer(m_DeviceInfo.Device, &bufferInfo, nullptr, &newBuffer->m_Buffer) !=
        VK_SUCCESS)
    {
        m_Logger.Error("Failed to create staging buffer.");
        delete newBuffer;
        return nullptr;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_DeviceInfo.Device, newBuffer->m_Buffer, &memRequirements);

    // TODO: allocation needs to be rewritten, cache mem type/index
    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits,
                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                          m_DeviceInfo.PhysicalDevice),
    };

    if (vkAllocateMemory(m_DeviceInfo.Device, &allocInfo, nullptr, &newBuffer->m_Memory) !=
        VK_SUCCESS)
    {
        m_Logger.Error("Failed to allocate memory for new staging buffer.");
        delete newBuffer;
        return nullptr;
    }

    vkBindBufferMemory(m_DeviceInfo.Device, newBuffer->m_Buffer, newBuffer->m_Memory, 0);
    return newBuffer;
}

void Engine::Core::Runtime::GraphicsLayer::DestroyStagingBuffer(Rendering::StagingBuffer *buffer)
{
    vkDestroyBuffer(m_DeviceInfo.Device, buffer->m_Buffer, nullptr);
    vkFreeMemory(m_DeviceInfo.Device, buffer->m_Memory, nullptr);
}

bool GraphicsLayer::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, size_t *srcOffsets,
                               size_t *dstOffsets, size_t *lengths, size_t segmentCount)
{
    // wait for the previous operation to be done
    vkWaitForFences(m_DeviceInfo.Device, 1, &m_TransferUtils.TransferFence, VK_TRUE, UINT64_MAX);

    VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr,
    };
    if (vkBeginCommandBuffer(m_TransferUtils.TransferCmdBuffer, &beginInfo) != VK_SUCCESS)
    {
        m_Logger.Error("Failed to begin transfer command buffer.");
        return false;
    }

    for (size_t i = 0; i < segmentCount; i++)
    {
        VkBufferCopy copyRegion{
            .srcOffset = srcOffsets[i],
            .dstOffset = dstOffsets[i],
            .size = lengths[i],
        };
        vkCmdCopyBuffer(m_TransferUtils.TransferCmdBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    }

    vkEndCommandBuffer(m_TransferUtils.TransferCmdBuffer);

    VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &m_TransferUtils.TransferCmdBuffer,
    };
    vkQueueSubmit(m_DeviceInfo.TransferQueue, 1, &submitInfo, m_TransferUtils.TransferFence);
    return true;
}

uint32_t Engine::Core::Runtime::GraphicsLayer::UploadGeometry(
    Rendering::StagingBuffer *stagingBuffer, size_t *vertexBufferOffsets,
    size_t *vertexBufferLengths, uint32_t vertexBufferCount, size_t indexBufferOffset,
    size_t indexBufferLength, Rendering::IndexType indexType, uint32_t indexCount)
{
    GpuGeometry geometry{
        .VertexBufferCount = vertexBufferCount,
        .IndexCount = indexCount,
        .IndexType = VK_INDEX_TYPE_NONE_KHR,
    };

    switch (indexType)
    {
    case IndexType::Ubyte:
        geometry.IndexType = VK_INDEX_TYPE_UINT8;
        break;
    case IndexType::Ushort:
        geometry.IndexType = VK_INDEX_TYPE_UINT16;
        break;
    case IndexType::Uint:
        geometry.IndexType = VK_INDEX_TYPE_UINT32;
        break;
    }

    if (vertexBufferCount > MaxVertexBufferBindings)
    {
        m_Logger.Error("Cannot create geometry with more than {} vertex buffer bindings.",
                       MaxVertexBufferBindings);
        return UINT32_MAX;
    }

    // calculate needed buffer size in total
    size_t bufferSize = indexBufferLength;
    for (size_t i = 0; i < vertexBufferCount; i++)
    {
        bufferSize += vertexBufferLengths[i];
    }

    // create a on-board buffer to hold all the data
    VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = bufferSize,
        .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    if (vkCreateBuffer(m_DeviceInfo.Device, &bufferInfo, nullptr, &geometry.Buffer) != VK_SUCCESS)
    {
        m_Logger.Error("Failed to create buffer for geometry.");
        return UINT32_MAX;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_DeviceInfo.Device, geometry.Buffer, &memRequirements);

    // TODO: cache memory type
    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex =
            FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                           m_DeviceInfo.PhysicalDevice),
    };

    if (vkAllocateMemory(m_DeviceInfo.Device, &allocInfo, nullptr, &geometry.Memory) != VK_SUCCESS)
    {
        m_Logger.Error("Failed to allocate memory for geometry.");
        vkDestroyBuffer(m_DeviceInfo.Device, geometry.Buffer, nullptr);
        return UINT32_MAX;
    }

    vkBindBufferMemory(m_DeviceInfo.Device, geometry.Buffer, geometry.Memory, 0);

    // prepare for the copy
    size_t srcOffsets[MaxVertexBufferBindings + 1];
    size_t dstOffsets[MaxVertexBufferBindings + 1];
    size_t lengths[MaxVertexBufferBindings + 1];

    size_t totalVertexSize = 0;
    for (size_t i = 0; i < vertexBufferCount; i++)
    {
        srcOffsets[i] = vertexBufferOffsets[i];
        dstOffsets[i] = totalVertexSize;
        lengths[i] = vertexBufferLengths[i];
        totalVertexSize += vertexBufferLengths[i];
        geometry.VertexBufferOffsets[i] = dstOffsets[i];
    }
    srcOffsets[vertexBufferCount] = indexBufferOffset;
    dstOffsets[vertexBufferCount] = totalVertexSize;
    lengths[vertexBufferCount] = indexBufferLength;
    geometry.IndexBufferOffset = totalVertexSize;

    // copy all segments of the staging buffer into the geometry buffer
    CopyBuffer(stagingBuffer->m_Buffer, geometry.Buffer, srcOffsets, dstOffsets, lengths,
               vertexBufferCount + 1);

    // register
    m_Geometries.push_back(geometry);
    return m_Geometries.size() - 1;
}
