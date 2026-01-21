#include "EngineCore/Rendering/Resources/device.h"
#include "../common.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Runtime/crash_dump.h"

#include "SDL3/SDL_vulkan.h"

#include <cstdint>
#include <memory>
#include <vulkan/vulkan_core.h>

using namespace Engine::Core::Rendering::Resources;
using namespace Engine::Core::Rendering;

constexpr const char *validationLayers[] = {"VK_LAYER_KHRONOS_validation"};

static VKAPI_ATTR VkBool32 VKAPI_CALL
VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
{
    Engine::Core::Logging::Logger *logger = (Engine::Core::Logging::Logger *)pUserData;

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

Engine::Core::Runtime::CallbackResult Device::Initialize(Engine::Core::Logging::Logger *logger,
                                                         bool enableValidationLayers,
                                                         SDL_Window *window)
{
    // set up m_Instance
    if (enableValidationLayers)
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
                return Runtime::Crash(__FILE__, __LINE__,
                                      std::string("Required validation layer not found: ") +
                                          neededLayer);
        }
    }

    vkEnumerateInstanceVersion(&m_Version);
    uint32_t majorVersion = VK_VERSION_MAJOR(m_Version);
    uint32_t minorVersion = VK_VERSION_MINOR(m_Version);

    if (m_Version < VK_VERSION_1_3)
        return Runtime::Crash(__FILE__, __LINE__,
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
        .enabledLayerCount =
            enableValidationLayers ? static_cast<uint32_t>(SDL_arraysize(validationLayers)) : 0,
        .ppEnabledLayerNames = validationLayers,
        .enabledExtensionCount = sdlExtensionCount,
        .ppEnabledExtensionNames = extensions,
    };

    CHECK_VULKAN(vkCreateInstance(&instanceInfo, nullptr, &m_Instance),
                 "Failed to create vulkan m_Instance!");

    // create surface
    if (!SDL_Vulkan_CreateSurface(window, m_Instance, nullptr, &m_Surface))
        return Runtime::Crash(__FILE__, __LINE__,
                              std::string("Failed to create vulkan surface, error: ") +
                                  SDL_GetError());

    // set up validation layers
    VkDebugUtilsMessengerCreateInfoEXT createInfo{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = VulkanDebugCallback,
        .pUserData = logger,
    };

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        m_Instance, "vkCreateDebugUtilsMessengerEXT");
    if (func == nullptr)
    {
        logger->Error("Failed to create debug messenger for Vulkan validation layers because "
                      "'vkCreateDebugUtilsMessengerEXT' can't be located.");
    }
    else if (func(m_Instance, &createInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS)
    {
        logger->Error("Failed to create debug messenger for Vulkan validation layers due to "
                      "Vulkan error.");
    }

    // get the physical device
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);

    if (deviceCount == 0)
        return Runtime::Crash(__FILE__, __LINE__, "Failed to find GPUs with Vulkan support.");

    std::unique_ptr<VkPhysicalDevice[]> physicalDevices =
        std::make_unique<VkPhysicalDevice[]>(deviceCount);
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, physicalDevices.get());

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

        // check queue availability by querying the
        bool hasGraphicsQueue = false;
        bool hasPresentQueue = false;
        bool hasTransferQueue = false;
        for (VkQueueFamilyProperties *queueFamily = queueFamilies.get();
             queueFamily < queueFamilies.get() + queueFamilyCount; queueFamily++)
        {
            uint32_t currentIndex = queueFamily - queueFamilies.get();

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(*physicalDevice, currentIndex, m_Surface,
                                                 &presentSupport);
            hasPresentQueue = hasPresentQueue || presentSupport;

            hasGraphicsQueue =
                hasGraphicsQueue || (queueFamily->queueFlags & VK_QUEUE_GRAPHICS_BIT) > 0;

            hasTransferQueue =
                hasTransferQueue || (queueFamily->queueFlags & VK_QUEUE_TRANSFER_BIT) > 0;

            if (hasGraphicsQueue && hasTransferQueue)
                break;
        }

        // break early if the device doesn't support the queues we need
        if (!hasGraphicsQueue || !hasPresentQueue || !hasTransferQueue)
            continue;

        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(*physicalDevice, nullptr, &extensionCount, nullptr);

        std::unique_ptr<VkExtensionProperties[]> availableExtensions =
            std::make_unique<VkExtensionProperties[]>(extensionCount);
        vkEnumerateDeviceExtensionProperties(*physicalDevice, nullptr, &extensionCount,
                                             availableExtensions.get());

        // check extensions
        bool allExtensionsSupported = true;
        for (const char *requiredExtension : requiredExtensions)
        {
            if (nullptr == SDL_bsearch(requiredExtensions, availableExtensions.get(),
                                       extensionCount, sizeof(VkExtensionProperties), CompareCStrs))
            {
                allExtensionsSupported = false;
                break;
            }
        }
        if (!allExtensionsSupported)
            continue;

        // to this point the device is confirmed, let's use it
        m_PhysicalDevice = *physicalDevice;
        break;
    }

    if (m_PhysicalDevice == VK_NULL_HANDLE)
        return Runtime::Crash(__FILE__, __LINE__,
                              "Failed to find an adaquate physical device that supports Vulkan.");

    // this whole ritualisitic dance just to get some well-known hardware functionalities is driving
    // me fucking nuts

    // pick the graphics queue and the transfer queue
    VqsQueueRequirements queueRequirements[] = {
        {VK_QUEUE_GRAPHICS_BIT, 1.0f, m_Surface},
        {VK_QUEUE_TRANSFER_BIT, 0.8f, nullptr},
    };

    VqsVulkanFunctions functions = {
        .vkGetPhysicalDeviceQueueFamilyProperties = vkGetPhysicalDeviceQueueFamilyProperties,
        .vkGetPhysicalDeviceSurfaceSupportKHR = vkGetPhysicalDeviceSurfaceSupportKHR,
    };

    VqsQueryCreateInfo queueQueryCreateInfo = {
        .physicalDevice = m_PhysicalDevice,
        .queueRequirementCount = SDL_arraysize(queueRequirements),
        .pQueueRequirements = queueRequirements,
        .pVulkanFunctions = &functions,
    };

    VqsQuery query;
    CHECK_VULKAN(vqsCreateQuery(&queueQueryCreateInfo, &query),
                 "Failed to create VQS queue selection query.");
    CHECK_VULKAN(vqsPerformQuery(query), "Failed to perform VQS queue selection query.");

    // get queue selections
    std::unique_ptr<VqsQueueSelection[]> selections =
        std::make_unique<VqsQueueSelection[]>(SDL_arraysize(queueRequirements));
    vqsGetQueueSelections(query, selections.get());

    // Get an array of VkDeviceQueueCreateInfo
    uint32_t queueCreateInfoCount, queuePriorityCount;
    vqsEnumerateDeviceQueueCreateInfos(query, &queueCreateInfoCount, nullptr, &queuePriorityCount,
                                       nullptr);

    std::unique_ptr<VkDeviceQueueCreateInfo[]> queueCreateInfos =
        std::make_unique<VkDeviceQueueCreateInfo[]>(queueCreateInfoCount);
    std::unique_ptr<float[]> queuePriorities = std::make_unique<float[]>(queuePriorityCount);

    vqsEnumerateDeviceQueueCreateInfos(query, &queueCreateInfoCount, queueCreateInfos.get(),
                                       &queuePriorityCount, queuePriorities.get());
    vqsDestroyQuery(query);

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
        .queueCreateInfoCount = queueCreateInfoCount,
        .pQueueCreateInfos = queueCreateInfos.get(),

        .enabledLayerCount =
            enableValidationLayers ? static_cast<uint32_t>(SDL_arraysize(validationLayers)) : 0,
        .ppEnabledLayerNames = enableValidationLayers ? validationLayers : nullptr,

        .enabledExtensionCount = SDL_arraysize(requiredExtensions),
        .ppEnabledExtensionNames = requiredExtensions,

        .pEnabledFeatures = &deviceFeatures,
    };

    CHECK_VULKAN(vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_LogicalDevice),
                 "Failed to create vulkan logical device.");

    m_GraphicsQueueSelection = selections[0];
    m_TransferQueueSelection = selections[1];

    vkGetDeviceQueue(m_LogicalDevice, selections[0].queueFamilyIndex, selections[0].queueIndex,
                     &m_GraphicsQueue);
    vkGetDeviceQueue(m_LogicalDevice, selections[0].presentQueueFamilyIndex,
                     selections[0].presentQueueIndex, &m_TransferQueue);
    vkGetDeviceQueue(m_LogicalDevice, selections[1].queueFamilyIndex, selections[1].queueIndex,
                     &m_TransferQueue);

    return Engine::Core::Runtime::CallbackSuccess();
}
